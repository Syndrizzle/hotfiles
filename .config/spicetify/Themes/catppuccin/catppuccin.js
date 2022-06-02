// Color map
let colorPalette = {
    rosewater: "#f5e0dc",
    flamingo: "#ddb6f2",
    pink: "#f5c2e7",
    maroon: "#e8a2af",
    red: "#f28fad",
    peach: "#f8bd96",
    yellow: "#fae3b0",
    green: "#abe9b3",
    teal: "#b5e8e0",
    blue: "#96cdfb",
    sky: "#89dceb",
    lavender: "#c9cbff",
    white: "#d9e0ee"
}

// waitForElement borrowed from:
// https://github.com/morpheusthewhite/spicetify-themes/blob/master/Dribbblish/dribbblish.js
function waitForElement(els, func, timeout = 100) {
    const queries = els.map(el => document.querySelector(el));
    if (queries.every(a => a)) {
      func(queries);
    } else if (timeout > 0) {
      setTimeout(waitForElement, 300, els, func, --timeout);
    }
}

// Return the color label for a given hex color value
function getKeyByValue(object, value) {
    return Object.keys(object).find(key => object[key] === value.trim());
}

// Used to select matching equalizer-animated-COLOR.gif
waitForElement([".Root"], (root) => {
    let spiceEq = getComputedStyle(document.querySelector(":root")).getPropertyValue("--spice-equalizer");
    let eqColor = getKeyByValue(colorPalette, spiceEq);
    root[0].classList.add(`catppuccin-eq-${eqColor}`);
});
