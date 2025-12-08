Time = 20
Dimensions = 3
Draw = "line"
Colors = {
    "blue",
    "orange",
    "white",
}

local data = {
    ["China"] = {
        k0 = 1.31,
        d = 0.07,
        l0 = 4.22,
        alpha = 0.68,
        theta = 0.25,
        omega = 15,
        s_mean = 0.425,
        n = -0.001,
        last = {
            k = 0,
            y = 0,
        }
    },
    ["India"] = {
        k0 = 0.62,
        d = 0.07,
        l0 = 4.28,
        alpha = 0.68,
        theta = 0.25,
        omega = 15,
        s_mean = 0.325,
        n = 0.007,
        last = {
            k = 0,
            y = 0,
        }
    },
    ["USA"] = {
        k0 = 1,
        d = 0.07,
        l0 = 1,
        alpha = 0.68,
        theta = 0.25,
        omega = 4,
        s_mean = 0.225,
        n = 0.004,
        last = {
            k = 0,
            y = 0,
        }
    },
}

local fns = {}

fns.labour = function(t, country)
    return country.l0 * math.exp(country.n * t)
end

fns.inv_rate = function(t, country)
    return country.s_mean * (1 + country.theta * math.sin(2 * math.pi * t / country.omega))
end

fns.capital = function(t, country)
    local k = fns.inv_rate(t - 1, country) * country.last.y + (1 - country.d) * country.last.k
    country.last.k = k
    return k
end

fns.production = function(t, country)
    local y
    if t <= 0 then
        country.last.k = country.k0
        y = (country.k0 ^ country.alpha) * (country.l0 ^ (1 - country.alpha))
    else
        y = (fns.capital(t, country) ^ country.alpha) * (fns.labour(t, country) ^ (1 - country.alpha))
    end
    country.last.y = y
    return y
end

-- Labour productivity function plot for 3 countries: China, India & USA.
-- Assuming that the USA is the base country (ie. K0_USA = 1, L0_USA = 1).
function Update(t)
    return {
        [1] = fns.production(t, data.China) / fns.labour(t, data.China),
        [2] = fns.production(t, data.India) / fns.labour(t, data.India),
        [3] = fns.production(t, data.USA) / fns.labour(t, data.USA),
    }
end
