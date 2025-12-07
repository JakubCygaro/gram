Time = 10
Dimensions = 1
Draw = "line"
Colors = {
    "blue",
}

local fib = { 0, 1 }

function Update(t)
    if t == 0 then
        return fib[1]
    elseif t == 1 then
        return fib[2]
    end
    local i = math.fmod(t, 2)
    fib[i+1] = fib[i+1] + fib[math.fmod(i + 1, 2) + 1]
    return fib[i+1]
end
