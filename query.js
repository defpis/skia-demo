export function queryLocalFontMeta() {
  const RATIO_AGENT_API_URL = "http://localhost:20710";
  return fetch(`${RATIO_AGENT_API_URL}/font-info`)
    .then((resp) => resp.json())
    .then((data) => {
      return Object.values(data).reduce((acc, cur) => {
        if (!acc[cur.family]) {
          acc[cur.family] = {};
        }

        const { family, style, weight, italic } = cur;

        const weights = acc[family];

        weights[cur.style] = {
          family,
          style,
          weight,
          italic,
          path: `${RATIO_AGENT_API_URL}/font-file?postscriptName=${cur.postscriptName}`,
        };

        return acc;
      }, {});
    })
    .catch((err) => {
      console.warn(err);
      return {};
    });
}

const FONT_WEIGHT_TO_STYLE = {
  100: ["Thin", "Hair Line"],
  200: ["Extra Light", "Ultra Light"],
  300: ["Light"],
  400: ["Regular", "Normal"],
  500: ["Medium"],
  600: ["Semi Bold", "Demi Bold"],
  700: ["Bold", "Wide"],
  800: ["Extra Bold", "Ultra Bold"],
  900: ["Black", "Heavy"],
};

function getFontInfoFromFontWeightStr(fontWeight) {
  let weight = 400;
  let italic = false;
  let style = "Regular";

  if (fontWeight === "italic") {
    italic = true;
    style = "Italic";
  } else {
    if (fontWeight !== "regular") {
      const [weightStr, styleStr] = fontWeight.match(/[a-z]+|[^a-z]+/gi);
      weight = Number(weightStr);
      italic = styleStr === "italic";
    }

    const styles = FONT_WEIGHT_TO_STYLE[weight];

    if (styles) {
      style = styles[0];
      style = `${style}${italic ? " Italic" : ""}`;
    } else {
      if (italic) {
        style = "Italic";
      } else {
        style = "Regular";
      }
    }
  }

  return {
    weight,
    italic,
    style,
  };
}

export function queryGoogleFontMeta() {
  const GOOGLE_FONT_API_URL =
    "https://www.googleapis.com/webfonts/v1/webfonts?key=";
  const GOOGLE_FONT_API_KEY = "AIzaSyAnXj5JcC6x0bJdHEdg_k6BitWCcpb39zw";
  return fetch(`${GOOGLE_FONT_API_URL}${GOOGLE_FONT_API_KEY}`)
    .then((resp) => resp.json())
    .then((data) => {
      return data.items.reduce((acc, cur) => {
        if (!acc[cur.family]) {
          acc[cur.family] = {};
        }

        const { family } = cur;

        const weights = acc[family];

        for (const [fontWeight, fileUrl] of Object.entries(cur.files)) {
          const { weight, italic, style } =
            getFontInfoFromFontWeightStr(fontWeight);

          weights[style] = {
            family,
            style,
            weight,
            italic,
            path: fileUrl.replace(/^http/, "https"),
          };
        }

        return acc;
      }, {});
    })
    .catch((err) => {
      console.warn(err);
      return {};
    });
}
