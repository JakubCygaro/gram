Time = 100
Dimensions = 1
Draw = "line"
Colors = {
    "blue",
}

function Update(t)
    local a = 10
    local b = 0.005
    local c = 0.1
    return b * math.exp(c * t * (a - c * t))
end
