#pragma once

#include <string>

namespace Http
{
	// timeoutMs у запросов - на весь запрос (соединение + обмен); 0 - обычные пределы (10 с / 60 с)

	// HTTPS GET. Возвращает HTTP-статус (0 — ошибка сети), тело — в body
	int Get(const std::string& host, const std::string& path, std::string& body, int timeoutMs = 0);

	// HTTPS POST (сертификат не проверяется). extraHeaders - доп. заголовки, каждый с \r\n на конце
	int Post(const std::string& host, const std::string& path, const std::string& contentType,
		const std::string& reqBody, const std::string& extraHeaders, std::string& body);
}
