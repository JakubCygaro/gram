Time = 0
Dimensions = 0
Draw = "rect"
Colors = {
    "orange",
}

local ceny;

function Init()
    ceny = Gram.load_csv("ceny.csv")
    Time = #ceny.rok
    Dimensions = ceny.dim
end

function Update(t)
    return ceny.rok[t+1] - 100
end
