Time = 100
Dimensions = 3
Draw = "line"
Colors = {
    "blue",
    "pink",
    "#f1f1f1"
}

function Update(t)
    return {
        [1] = math.sin(20 * t / Time),
        [2] = math.cos(20 * t / Time),
        [3] = math.atan(20 * t / Time),
    }
end
