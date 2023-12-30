-- 请求头
wrk.path = "/"
wrk.method = "GET"
wrk.headers["Connection"] = "keep-alive"