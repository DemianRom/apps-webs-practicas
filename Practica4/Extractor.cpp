#include "Extractor.hpp"
#include <regex>
#include <sstream>

using namespace std;

string normalizarHostPuerto(string host, const string &protocolo) {
  if (host.empty() || host[0] == '[') {
    return host;
  }
  size_t dosPuntos = host.find(':');
  if (dosPuntos == string::npos) {
    return host;
  }

  string puerto = host.substr(dosPuntos + 1);
  if ((protocolo == "http://" && puerto == "80") ||
      (protocolo == "https://" && puerto == "443")) {
    return host.substr(0, dosPuntos);
  }
  return host;
}

URLDesglosada desglosarURL(const string &url) {
  URLDesglosada res;
  size_t protoEnd = url.find("://");
  if (protoEnd == string::npos)
    return res;

  res.protocolo = url.substr(0, protoEnd + 3);
  string resto = url.substr(protoEnd + 3);

  size_t hostEnd = resto.find_first_of("/?#");
  if (hostEnd == string::npos) {
    res.host = resto;
    res.rutaCompleta = "/";
  } else {
    res.host = resto.substr(0, hostEnd);
    string cola = resto.substr(hostEnd);
    if (!cola.empty() && cola[0] == '/') {
      res.rutaCompleta = cola;
    } else {
      res.rutaCompleta = "/" + cola;
    }
  }
  res.host = normalizarHostPuerto(res.host, res.protocolo);
  return res;
}

string resolverRutaJerarquica(const string &rutaSucia) {
  vector<string> segmentos;
  stringstream ss(rutaSucia);
  string segmento;

  while (getline(ss, segmento, '/')) {
    if (segmento == "" || segmento == ".")
      continue;
    if (segmento == "..") {
      if (!segmentos.empty())
        segmentos.pop_back();
    } else {
      segmentos.push_back(segmento);
    }
  }

  string rutaLimpia = "";
  for (const string &seg : segmentos) {
    rutaLimpia += "/" + seg;
  }
  return rutaLimpia.empty() ? "/" : rutaLimpia;
}

string normalizarRutaPreservandoSlashFinal(const string &rutaSucia) {
  bool teniaSlashFinal = !rutaSucia.empty() && rutaSucia.back() == '/';
  string rutaNormalizada = resolverRutaJerarquica(rutaSucia);
  if (teniaSlashFinal && rutaNormalizada != "/") {
    rutaNormalizada += "/";
  }
  return rutaNormalizada;
}

string normalizarURLRobust(string urlDestino, const string &urlBaseOrigin) {
  size_t hashPos = urlDestino.find('#');
  if (hashPos != string::npos) {
    urlDestino = urlDestino.substr(0, hashPos);
  }

  if (urlDestino.empty())
    return "";

  if (urlDestino.rfind("http://", 0) == 0 ||
      urlDestino.rfind("https://", 0) == 0) {
    URLDesglosada d = desglosarURL(urlDestino);
    if (d.protocolo.empty() || d.host.empty())
      return "";
    size_t queryPos = d.rutaCompleta.find('?');
    string pathPart = (queryPos == string::npos)
                          ? d.rutaCompleta
                          : d.rutaCompleta.substr(0, queryPos);
    string queryPart =
        (queryPos == string::npos) ? "" : d.rutaCompleta.substr(queryPos);
    return d.protocolo + d.host + normalizarRutaPreservandoSlashFinal(pathPart) +
           queryPart;
  }

  size_t colonPos = urlDestino.find(':');
  size_t slashPos = urlDestino.find('/');
  if (colonPos != string::npos &&
      (slashPos == string::npos || colonPos < slashPos)) {
    return "";
  }

  URLDesglosada base = desglosarURL(urlBaseOrigin);

  if (urlDestino.rfind("//", 0) == 0) {
    return normalizarURLRobust(base.protocolo + urlDestino.substr(2),
                               urlBaseOrigin);
  }

  if (urlDestino.rfind("/", 0) == 0) {
    size_t queryPos = urlDestino.find('?');
    string pathPart = (queryPos == string::npos)
                          ? urlDestino
                          : urlDestino.substr(0, queryPos);
    string queryPart =
        (queryPos == string::npos) ? "" : urlDestino.substr(queryPos);
    return base.protocolo + base.host +
           normalizarRutaPreservandoSlashFinal(pathPart) + queryPart;
  }

  size_t ultimoSlash = base.rutaCompleta.find_last_of('/');
  string dirActual = base.rutaCompleta.substr(0, ultimoSlash + 1);

  size_t queryPos = urlDestino.find('?');
  string pathPart =
      (queryPos == string::npos) ? urlDestino : urlDestino.substr(0, queryPos);
  string queryPart =
      (queryPos == string::npos) ? "" : urlDestino.substr(queryPos);

  string rutaCombinada = dirActual + pathPart;
  return base.protocolo + base.host +
         normalizarRutaPreservandoSlashFinal(rutaCombinada) + queryPart;
}

string convertirURLaNombreArchivo(const string &url) {
  string nombre = url;
  for (char &c : nombre) {
    if (c == '/' || c == ':' || c == '?' || c == '=' || c == '&' || c == '*') {
      c = '_';
    }
  }
  return nombre;
}

vector<string> extraerEnlaces(const string &html, const string &urlBaseOrigin) {
  vector<string> enlacesValidados;
  regex tokenRegex(
      R"URL((?:href|src)\s*=\s*(?:"([^"]+)"|'([^']+)'|([^"'\s>]+)))URL",
      regex_constants::icase);

  auto palabras_begin = sregex_iterator(html.begin(), html.end(), tokenRegex);
  auto palabras_end = sregex_iterator();

  for (sregex_iterator i = palabras_begin; i != palabras_end; ++i) {
    smatch match = *i;
    string urlExtraida = match[1].matched
                             ? match[1].str()
                             : (match[2].matched ? match[2].str() : match[3].str());

    if (urlExtraida.empty() || urlExtraida[0] == '#' ||
        urlExtraida.rfind("javascript:", 0) == 0 ||
        urlExtraida.rfind("mailto:", 0) == 0 ||
        urlExtraida.rfind("tel:", 0) == 0 ||
        urlExtraida.rfind("data:", 0) == 0) {
      continue;
    }

    string urlAbsoluta = normalizarURLRobust(urlExtraida, urlBaseOrigin);
    if (!urlAbsoluta.empty()) {
      enlacesValidados.push_back(urlAbsoluta);
    }
  }
  return enlacesValidados;
}
