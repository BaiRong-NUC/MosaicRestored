#pragma once
#include <cstddef>
#include <string>

// R"html(... )html" 是 C++ 的原始字符串字面量

// 嵌入的 404 页面，用于编译进静态库
inline constexpr char embedded_404[] = R"html(<!doctype html>
<html lang="zh-CN">
<head>
	<meta charset="utf-8">
	<meta name="viewport" content="width=device-width, initial-scale=1">
	<title>404 Not Found</title>
	<style>
		body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial; color:#333; background:#f8f9fb; margin:0; }
		.wrap { max-width:720px; margin:80px auto; text-align:center; }
		h1 { font-size:72px; margin:0; }
		p { font-size:18px; color:#666; }
		a { color:#007bff; text-decoration:none; }
	</style>
</head>
<body>
	<div class="wrap">
		<h1>404</h1>
		<p>抱歉，未找到您请求的资源。</p>
		<p><a href="/">返回首页</a></p>
	</div>
</body>
</html>)html";

inline constexpr std::size_t embedded_404_size = sizeof(embedded_404) - 1;

inline std::string GetEmbedded404() { return std::string(embedded_404, embedded_404_size); }

// 嵌入的 405 页面，用于编译进静态库
inline constexpr char embedded_405[] = R"html(<!doctype html>
<html lang="zh-CN">
<head>
		<meta charset="utf-8">
		<meta name="viewport" content="width=device-width, initial-scale=1">
		<title>405 Method Not Allowed</title>
		<style>
			body { font-family: -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial; color:#333; background:#fff; margin:0; }
			.wrap{ max-width:720px; margin:80px auto; text-align:center; }
			h1{ font-size:56px; margin:0; }
			p{ font-size:18px; color:#666; }
		</style>
</head>
<body>
	<div class="wrap">
		<h1>405</h1>
		<p>不允许的请求方法(Method Not Allowed)。</p>
		<p>请检查请求方法或查看 API 文档。</p>
	</div>
</body>
</html>)html";

inline constexpr std::size_t embedded_405_size = sizeof(embedded_405) - 1;

inline std::string GetEmbedded405() { return std::string(embedded_405, embedded_405_size); }

// 500与通用错误页面（可用于其他未指定的错误）
inline constexpr char embedded_error[] = R"html(<!doctype html>
<html lang="zh-CN">
<head>
		<meta charset="utf-8">
		<meta name="viewport" content="width=device-width, initial-scale=1">
		<title>服务器错误</title>
		<style>
			body { font-family: -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial; color:#333; background:#fff; margin:0; }
			.wrap{ max-width:720px; margin:80px auto; text-align:center; }
			h1{ font-size:48px; margin:0; }
			p{ font-size:16px; color:#666; }
		</style>
</head>
<body>
	<div class="wrap">
		<h1>出现服务器错误</h1>
		<p>发生了未知错误，服务器无法完成请求。</p>
	</div>
</body>
</html>)html";

inline constexpr std::size_t embedded_error_size = sizeof(embedded_error) - 1;

inline std::string GetEmbeddedError() { return std::string(embedded_error, embedded_error_size); }