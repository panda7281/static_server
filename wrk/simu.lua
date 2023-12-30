local request_array = {}

-- init: 生成请求头
function init()
    for i=1, 50 do
        request_array[i] = wrk.format("GET", "/imgs/" .. tostring(i) .. ".jpg", {Connection="keep-alive"}, nil)
    end
end

function request()
   request_index = math.random(1, 50)
   return request_array[request_index]
end

-- 每个请求间隔10-50ms的随机延时
function delay()
    return math.random(10, 50)
end