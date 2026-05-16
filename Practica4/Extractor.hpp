#ifndef EXTRACTOR_HPP
#define EXTRACTOR_HPP

#include <string>
#include <vector>

struct URLDesglosada {
  std::string protocolo;
  std::string host;
  std::string rutaCompleta;
};

URLDesglosada desglosarURL(const std::string &url);
std::string resolverRutaJerarquica(const std::string &rutaSucia);
std::string normalizarURLRobust(std::string urlDestino,
                                const std::string &urlBaseOrigin);
std::string convertirURLaNombreArchivo(const std::string &url);
std::vector<std::string> extraerEnlaces(const std::string &html,
                                        const std::string &urlBaseOrigin);

#endif
