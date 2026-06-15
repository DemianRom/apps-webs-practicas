/*
 * Practica 4 - Web crawler y mirror local
 * Materia: Aplicaciones y Comunicaciones en Red (6CM1)
 * ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
 * Periodo: 26/2
 *
 * Integrantes:
 * - Romero Bautista Demian
 * - Ferreira Rodriguez Said
 *
 * Responsabilidad del archivo:
 * Contiene la logica principal del crawler: descarga HTTP, pool de hilos, frontera compartida y guardado local.
 */

#include "Extractor.hpp"
#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <regex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct ContextoDescarga {
  string body;
  string contentType;
  string urlFinal;
  long httpCode = 0;
};

size_t CallbackBody(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t tamanioTotal = size * nmemb;
  ContextoDescarga *ctx = static_cast<ContextoDescarga *>(userp);
  ctx->body.append(static_cast<char *>(contents), tamanioTotal);
  return tamanioTotal;
}

bool ejecutarPeticion(const string &url, ContextoDescarga &ctx) {
  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  ctx.body.clear();
  ctx.contentType.clear();
  ctx.urlFinal = url;
  ctx.httpCode = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Practica4Crawler/2.0");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CallbackBody);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

  CURLcode rc = curl_easy_perform(curl);
  if (rc != CURLE_OK) {
    cerr << "Fallo en la transferencia: " << curl_easy_strerror(rc) << '\n';
    curl_easy_cleanup(curl);
    return false;
  }

  char *contentType = nullptr;
  char *effectiveUrl = nullptr;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
  curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &ctx.httpCode);

  ctx.contentType = contentType ? contentType : "";
  if (effectiveUrl) {
    ctx.urlFinal = effectiveUrl;
  }

  curl_easy_cleanup(curl);
  return true;
}

string aMinusculas(string texto) {
  transform(texto.begin(), texto.end(), texto.begin(),
            [](unsigned char c) { return static_cast<char>(tolower(c)); });
  return texto;
}

string hostComparable(string host) {
  host = aMinusculas(host);
  if (host.rfind("[", 0) == 0) {
    return host;
  }
  size_t dosPuntos = host.find(':');
  if (dosPuntos != string::npos) {
    string puerto = host.substr(dosPuntos + 1);
    if (puerto == "80" || puerto == "443") {
      host = host.substr(0, dosPuntos);
    }
  }
  return host;
}

string sanitizarComponente(const string &texto) {
  string limpio;
  limpio.reserve(texto.size());
  for (unsigned char c : texto) {
    if (isalnum(c) || c == '.' || c == '-' || c == '_') {
      limpio.push_back(static_cast<char>(c));
    } else {
      limpio.push_back('_');
    }
  }
  return limpio.empty() ? "_" : limpio;
}

bool tieneExtension(const string &segmento) {
  size_t punto = segmento.find_last_of('.');
  return punto != string::npos && punto != 0 && punto + 1 < segmento.size();
}

string contentTypeLower(const string &contentType) {
  string lower = contentType;
  transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(tolower(c)); });
  return lower;
}

bool esHTML(const string &contentType) {
  string t = contentTypeLower(contentType);
  return t.find("text/html") != string::npos ||
         t.find("application/xhtml+xml") != string::npos;
}

bool esCSS(const string &contentType) {
  return contentTypeLower(contentType).find("text/css") != string::npos;
}

bool esJS(const string &contentType) {
  string t = contentTypeLower(contentType);
  return t.find("javascript") != string::npos || t.find("ecmascript") != string::npos;
}

bool esTextoParseable(const string &contentType) {
  string t = contentTypeLower(contentType);
  return esHTML(t) || esCSS(t) || esJS(t) || t.find("json") != string::npos ||
         t.find("xml") != string::npos || t.find("svg") != string::npos ||
         t.rfind("text/", 0) == 0;
}

fs::path construirRutaLocal(const string &url, const fs::path &raiz) {
  URLDesglosada d = desglosarURL(url);
  string ruta = d.rutaCompleta.empty() ? "/" : d.rutaCompleta;

  size_t queryPos = ruta.find('?');
  string pathPart = (queryPos == string::npos) ? ruta : ruta.substr(0, queryPos);
  string queryPart = (queryPos == string::npos) ? "" : ruta.substr(queryPos + 1);

  pathPart = resolverRutaJerarquica(pathPart);
  if (pathPart.empty() || pathPart[0] != '/') {
    pathPart = "/" + pathPart;
  }

  vector<string> segmentos;
  size_t inicio = 1;
  while (inicio <= pathPart.size()) {
    size_t fin = pathPart.find('/', inicio);
    string segmento = (fin == string::npos)
                          ? pathPart.substr(inicio)
                          : pathPart.substr(inicio, fin - inicio);
    if (!segmento.empty()) {
      segmentos.push_back(sanitizarComponente(segmento));
    }
    if (fin == string::npos) {
      break;
    }
    inicio = fin + 1;
  }

  bool terminaConSlash = !pathPart.empty() && pathPart.back() == '/';
  if (segmentos.empty()) {
    segmentos.push_back("index.html");
  } else {
    string &ultimo = segmentos.back();
    if (terminaConSlash || !tieneExtension(ultimo)) {
      segmentos.push_back("index.html");
    }
  }

  if (!queryPart.empty()) {
    string querySanitizada = sanitizarComponente(queryPart);
    string &archivo = segmentos.back();
    size_t punto = archivo.find_last_of('.');
    if (punto == string::npos) {
      archivo += "__q_" + querySanitizada;
    } else {
      archivo.insert(punto, "__q_" + querySanitizada);
    }
  }

  fs::path salida = raiz;
  for (const string &seg : segmentos) {
    salida /= seg;
  }
  return salida;
}

string convertirRutaRelativa(const fs::path &desdeArchivo,
                             const fs::path &haciaArchivo) {
  fs::path relativa = fs::relative(haciaArchivo, desdeArchivo.parent_path());
  string ruta = relativa.generic_string();
  return ruta.empty() ? "./" : ruta;
}

vector<string> extraerConRegex(const string &texto, const regex &patron,
                               int grupo = 1) {
  vector<string> encontrados;
  auto begin = sregex_iterator(texto.begin(), texto.end(), patron);
  auto end = sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    string valor = (*it)[grupo].str();
    if (!valor.empty()) {
      encontrados.push_back(valor);
    }
  }
  return encontrados;
}

vector<string> extraerUrlsHTMLAmplio(const string &html) {
  vector<string> urls;

  regex attrs(
      R"URL((?:href|src|poster|data-src|data-href)\s*=\s*(?:"([^"]+)"|'([^']+)'|([^"'\s>]+)))URL",
      regex_constants::icase);
  auto attrsBegin = sregex_iterator(html.begin(), html.end(), attrs);
  auto attrsEnd = sregex_iterator();
  for (auto it = attrsBegin; it != attrsEnd; ++it) {
    const smatch &match = *it;
    string valor = match[1].matched
                       ? match[1].str()
                       : (match[2].matched ? match[2].str() : match[3].str());
    if (!valor.empty()) {
      urls.push_back(valor);
    }
  }

  regex srcsetRegex(R"(srcset\s*=\s*["']([^"']+)["'])", regex_constants::icase);
  vector<string> srcsets = extraerConRegex(html, srcsetRegex);
  for (const string &srcset : srcsets) {
    size_t inicio = 0;
    while (inicio < srcset.size()) {
      size_t coma = srcset.find(',', inicio);
      string item = (coma == string::npos)
                        ? srcset.substr(inicio)
                        : srcset.substr(inicio, coma - inicio);
      size_t primerNoEspacio = item.find_first_not_of(" \t\r\n");
      if (primerNoEspacio != string::npos) {
        item = item.substr(primerNoEspacio);
        size_t separador = item.find_first_of(" \t\r\n");
        string enlace = (separador == string::npos) ? item : item.substr(0, separador);
        if (!enlace.empty()) {
          urls.push_back(enlace);
        }
      }
      if (coma == string::npos) {
        break;
      }
      inicio = coma + 1;
    }
  }

  regex cssUrl(R"(url\(\s*['"]?([^'")]+)['"]?\s*\))", regex_constants::icase);
  vector<string> deCss = extraerConRegex(html, cssUrl);
  urls.insert(urls.end(), deCss.begin(), deCss.end());

  return urls;
}

vector<string> extraerUrlsCSS(const string &css) {
  vector<string> urls;
  regex cssUrl(R"(url\(\s*['"]?([^'")]+)['"]?\s*\))", regex_constants::icase);
  vector<string> deCss = extraerConRegex(css, cssUrl);
  urls.insert(urls.end(), deCss.begin(), deCss.end());

  regex importCss(R"(@import\s+(?:url\()?['"]?([^'")\s]+)['"]?\)?)",
                  regex_constants::icase);
  vector<string> deImport = extraerConRegex(css, importCss);
  urls.insert(urls.end(), deImport.begin(), deImport.end());
  return urls;
}

vector<string> extraerUrlsTextoGeneral(const string &texto) {
  vector<string> urls;
  regex absolutas(R"(["'](https?://[^"'<> \t\r\n]+)["'])", regex_constants::icase);
  vector<string> deAbsolutas = extraerConRegex(texto, absolutas);
  urls.insert(urls.end(), deAbsolutas.begin(), deAbsolutas.end());

  regex relativasRoot(R"(["'](/[^"'<> \t\r\n]+)["'])", regex_constants::icase);
  vector<string> deRelativas = extraerConRegex(texto, relativasRoot);
  urls.insert(urls.end(), deRelativas.begin(), deRelativas.end());
  return urls;
}

string reescribirCoincidencias(const string &texto, const string &urlBaseActual,
                               const string &hostBase, const fs::path &archivoActual,
                               const fs::path &raizSalida, const regex &patron,
                               int grupoValor) {
  string resultado;
  size_t ultimo = 0;
  auto begin = sregex_iterator(texto.begin(), texto.end(), patron);
  auto end = sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    const smatch &match = *it;
    size_t inicio = static_cast<size_t>(match.position());
    size_t largo = static_cast<size_t>(match.length());
    resultado.append(texto, ultimo, inicio - ultimo);

    string original = match.str();
    string valor = match[grupoValor].str();
    string reemplazo = original;

    string urlNormalizada = normalizarURLRobust(valor, urlBaseActual);
    if (!urlNormalizada.empty()) {
      URLDesglosada d = desglosarURL(urlNormalizada);
      if (!d.host.empty() && hostComparable(d.host) == hostBase) {
        fs::path destino = construirRutaLocal(urlNormalizada, raizSalida);
        string relativo = convertirRutaRelativa(archivoActual, destino);
        size_t pos = reemplazo.find(valor);
        if (pos != string::npos) {
          reemplazo.replace(pos, valor.size(), relativo);
        }
      }
    }

    resultado += reemplazo;
    ultimo = inicio + largo;
  }
  resultado.append(texto, ultimo, string::npos);
  return resultado;
}

string reescribirHTML(const string &html, const string &urlBaseActual,
                      const string &hostBase, const fs::path &archivoActual,
                      const fs::path &raizSalida) {
  regex attrs(
      R"((?:href|src|poster|data-src|data-href)\s*=\s*["']([^"']+)["'])",
      regex_constants::icase);
  string salida =
      reescribirCoincidencias(html, urlBaseActual, hostBase, archivoActual,
                              raizSalida, attrs, 1);

  regex cssUrl(R"(url\(\s*['"]?([^'")]+)['"]?\s*\))", regex_constants::icase);
  salida = reescribirCoincidencias(salida, urlBaseActual, hostBase, archivoActual,
                                   raizSalida, cssUrl, 1);

  regex srcsetRegex(R"(srcset\s*=\s*["']([^"']+)["'])", regex_constants::icase);
  auto begin = sregex_iterator(salida.begin(), salida.end(), srcsetRegex);
  auto end = sregex_iterator();
  string finalHtml;
  size_t ultimo = 0;
  for (auto it = begin; it != end; ++it) {
    const smatch &match = *it;
    size_t inicio = static_cast<size_t>(match.position());
    size_t largo = static_cast<size_t>(match.length());
    finalHtml.append(salida, ultimo, inicio - ultimo);

    string original = match.str();
    string valor = match[1].str();
    string nuevoValor;
    size_t pos = 0;
    while (pos < valor.size()) {
      size_t coma = valor.find(',', pos);
      string item = (coma == string::npos) ? valor.substr(pos)
                                           : valor.substr(pos, coma - pos);
      string separadorFinal = (coma == string::npos) ? "" : ",";
      size_t inicioNoEspacio = item.find_first_not_of(" \t\r\n");
      if (inicioNoEspacio == string::npos) {
        nuevoValor += item + separadorFinal;
      } else {
        string prefijoEspacios = item.substr(0, inicioNoEspacio);
        string resto = item.substr(inicioNoEspacio);
        size_t separador = resto.find_first_of(" \t\r\n");
        string urlToken = (separador == string::npos) ? resto : resto.substr(0, separador);
        string descriptor = (separador == string::npos) ? "" : resto.substr(separador);

        string urlNormalizada = normalizarURLRobust(urlToken, urlBaseActual);
        string urlFinal = urlToken;
        if (!urlNormalizada.empty()) {
          URLDesglosada d = desglosarURL(urlNormalizada);
          if (!d.host.empty() && hostComparable(d.host) == hostBase) {
            fs::path destino = construirRutaLocal(urlNormalizada, raizSalida);
            urlFinal = convertirRutaRelativa(archivoActual, destino);
          }
        }
        nuevoValor += prefijoEspacios + urlFinal + descriptor + separadorFinal;
      }
      if (coma == string::npos) {
        break;
      }
      pos = coma + 1;
    }

    size_t inicioValor = original.find(valor);
    string reemplazo = original;
    if (inicioValor != string::npos) {
      reemplazo.replace(inicioValor, valor.size(), nuevoValor);
    }
    finalHtml += reemplazo;
    ultimo = inicio + largo;
  }
  finalHtml.append(salida, ultimo, string::npos);
  return finalHtml;
}

string reescribirCSS(const string &css, const string &urlBaseActual,
                     const string &hostBase, const fs::path &archivoActual,
                     const fs::path &raizSalida) {
  regex cssUrl(R"(url\(\s*['"]?([^'")]+)['"]?\s*\))", regex_constants::icase);
  string salida = reescribirCoincidencias(css, urlBaseActual, hostBase, archivoActual,
                                          raizSalida, cssUrl, 1);
  regex importCss(R"(@import\s+(?:url\()?['"]?([^'")\s]+)['"]?\)?)",
                  regex_constants::icase);
  return reescribirCoincidencias(salida, urlBaseActual, hostBase, archivoActual,
                                 raizSalida, importCss, 1);
}

bool guardarArchivo(const fs::path &ruta, const string &contenido) {
  fs::create_directories(ruta.parent_path());
  ofstream salida(ruta, ios::binary);
  if (!salida.is_open()) {
    return false;
  }
  salida.write(contenido.data(), static_cast<std::streamsize>(contenido.size()));
  return salida.good();
}

class FronteraCrawler {
public:
  bool encolarSiNuevo(const string &url) {
    lock_guard<mutex> lock(mutex_);
    if (detenido_) {
      return false;
    }
    if (visitados_.insert(url).second) {
      cola_.push(url);
      cv_.notify_one();
      return true;
    }
    return false;
  }

  bool tomarSiguiente(string &url) {
    unique_lock<mutex> lock(mutex_);
    cv_.wait(lock, [&] {
      return detenido_ || !cola_.empty() || (cola_.empty() && activos_ == 0);
    });

    if (detenido_) {
      return false;
    }

    if (cola_.empty() && activos_ == 0) {
      detenido_ = true;
      cv_.notify_all();
      return false;
    }

    if (cola_.empty()) {
      return false;
    }

    url = cola_.front();
    cola_.pop();
    ++activos_;
    return true;
  }

  void marcarFinTrabajo() {
    lock_guard<mutex> lock(mutex_);
    if (activos_ > 0) {
      --activos_;
    }
    if (cola_.empty() && activos_ == 0) {
      detenido_ = true;
      cv_.notify_all();
    }
  }

private:
  queue<string> cola_;
  set<string> visitados_;
  size_t activos_ = 0;
  bool detenido_ = false;
  mutex mutex_;
  condition_variable cv_;
};

string normalizarDelMismoHost(const string &urlRaw, const string &baseUrl,
                              const string &hostBase) {
  string normalizada = normalizarURLRobust(urlRaw, baseUrl);
  if (normalizada.empty()) {
    return "";
  }

  URLDesglosada d = desglosarURL(normalizada);
  if (d.host.empty() || hostComparable(d.host) != hostBase) {
    return "";
  }
  return normalizada;
}

mutex gLogMutex;

void logSeguro(ostream &salida, const string &mensaje) {
  lock_guard<mutex> lock(gLogMutex);
  salida << mensaje << endl;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Uso: " << argv[0]
         << " <URL_INICIAL> [DIRECTORIO_SALIDA] [NUM_HILOS]" << endl;
    return 1;
  }

  string urlInicial = argv[1];
  string directorioSalida = (argc >= 3) ? argv[2] : "mirror";
  unsigned int numHilos = (argc >= 4) ? static_cast<unsigned int>(strtoul(argv[3], nullptr, 10))
                                      : thread::hardware_concurrency();
  if (numHilos == 0) {
    numHilos = 4;
  }
  URLDesglosada base = desglosarURL(urlInicial);
  if ((aMinusculas(base.protocolo) != "http://" &&
       aMinusculas(base.protocolo) != "https://") ||
      base.host.empty()) {
    cerr << "URL inicial inválida. Debe incluir http:// o https://." << endl;
    return 1;
  }

  string hostBase = hostComparable(base.host);
  string urlInicialNormalizada = normalizarURLRobust(urlInicial, urlInicial);
  if (urlInicialNormalizada.empty()) {
    cerr << "No se pudo normalizar la URL inicial." << endl;
    return 1;
  }

  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    cerr << "No se pudo inicializar libcurl." << endl;
    return 1;
  }

  fs::path raizSalida(directorioSalida);
  fs::create_directories(raizSalida);

  FronteraCrawler frontera;
  frontera.encolarSiNuevo(urlInicialNormalizada);

  vector<thread> workers;
  workers.reserve(numHilos);
  for (unsigned int i = 0; i < numHilos; ++i) {
    workers.emplace_back([&, i]() {
      string urlActual;
      while (frontera.tomarSiguiente(urlActual)) {
        ContextoDescarga resultado;
        logSeguro(cout, "[T" + to_string(i + 1) + "] [CRAWL] Descargando: " +
                            urlActual);

        if (!ejecutarPeticion(urlActual, resultado)) {
          logSeguro(cerr, "[T" + to_string(i + 1) +
                             "] [ERROR] No se pudo descargar: " + urlActual);
          frontera.marcarFinTrabajo();
          continue;
        }

        string urlCanonical = normalizarDelMismoHost(resultado.urlFinal, urlActual, hostBase);
        if (urlCanonical.empty()) {
          frontera.marcarFinTrabajo();
          continue;
        }

        frontera.encolarSiNuevo(urlCanonical);

        if (resultado.httpCode >= 400) {
          logSeguro(cerr, "[T" + to_string(i + 1) + "] [WARN] HTTP " +
                             to_string(resultado.httpCode) + " en " + urlActual);
          frontera.marcarFinTrabajo();
          continue;
        }

        fs::path archivoLocal = construirRutaLocal(urlCanonical, raizSalida);
        string contenidoParaGuardar = resultado.body;
        if (esHTML(resultado.contentType)) {
          contenidoParaGuardar = reescribirHTML(resultado.body, urlCanonical, hostBase,
                                                archivoLocal, raizSalida);
        } else if (esCSS(resultado.contentType)) {
          contenidoParaGuardar = reescribirCSS(resultado.body, urlCanonical, hostBase,
                                               archivoLocal, raizSalida);
        }

        if (!guardarArchivo(archivoLocal, contenidoParaGuardar)) {
          logSeguro(cerr, "[T" + to_string(i + 1) +
                             "] [ERROR] No se pudo guardar recurso en " +
                             archivoLocal.string());
          frontera.marcarFinTrabajo();
          continue;
        }

        vector<string> encontradas;
        if (esHTML(resultado.contentType)) {
          vector<string> linksBase = extraerEnlaces(resultado.body, urlCanonical);
          vector<string> linksAmplio = extraerUrlsHTMLAmplio(resultado.body);
          vector<string> linksTexto = extraerUrlsTextoGeneral(resultado.body);
          encontradas.insert(encontradas.end(), linksBase.begin(), linksBase.end());
          encontradas.insert(encontradas.end(), linksAmplio.begin(), linksAmplio.end());
          encontradas.insert(encontradas.end(), linksTexto.begin(), linksTexto.end());
          logSeguro(cout, "[T" + to_string(i + 1) + "] [HTML] Guardado: " +
                             archivoLocal.string());
        } else if (esCSS(resultado.contentType)) {
          vector<string> linksCss = extraerUrlsCSS(resultado.body);
          vector<string> linksTexto = extraerUrlsTextoGeneral(resultado.body);
          encontradas.insert(encontradas.end(), linksCss.begin(), linksCss.end());
          encontradas.insert(encontradas.end(), linksTexto.begin(), linksTexto.end());
          logSeguro(cout, "[T" + to_string(i + 1) + "] [CSS] Guardado: " +
                             archivoLocal.string());
        } else if (esJS(resultado.contentType) || esTextoParseable(resultado.contentType)) {
          vector<string> linksTexto = extraerUrlsTextoGeneral(resultado.body);
          encontradas.insert(encontradas.end(), linksTexto.begin(), linksTexto.end());
          logSeguro(cout, "[T" + to_string(i + 1) + "] [TXT] Guardado: " +
                             archivoLocal.string());
        } else {
          logSeguro(cout, "[T" + to_string(i + 1) + "] [BIN] Guardado: " +
                             archivoLocal.string());
        }

        for (const string &urlEncontrada : encontradas) {
          string normalizada = normalizarDelMismoHost(urlEncontrada, urlCanonical, hostBase);
          if (!normalizada.empty()) {
            frontera.encolarSiNuevo(normalizada);
          }
        }

        frontera.marcarFinTrabajo();
      }
    });
  }

  for (thread &worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }

  curl_global_cleanup();
  return 0;
}
