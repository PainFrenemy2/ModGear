#pragma once

// Папка сервера (где лежит samp-server / omp-server) с разделителем на конце
// Пути от неё не зависят от рабочей папки процесса (панели и скрипты запуска могут её менять)

#include <string>

const std::string& ServerDir();

// путь от папки сервера: ServerPath("scriptfiles/x.dat")
inline std::string ServerPath(const std::string& relative) { return ServerDir() + relative; }
