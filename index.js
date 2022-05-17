import Module from "./out/skia_demo.js";
import { queryGoogleFontMeta, queryLocalFontMeta } from "./query.js";

const rootElem = document.querySelector("#root");

const appElem = document.createElement("canvas");
appElem.id = "app";
appElem.style.width = "100%";
appElem.style.height = "100%";

rootElem.appendChild(appElem);

const appResizeObs = new ResizeObserver((entries) => {
  entries.forEach(({ contentRect: { width, height } }) => {
    appElem.width = width * window.devicePixelRatio;
    appElem.height = height * window.devicePixelRatio;
  });
});
appResizeObs.observe(rootElem);

const encoder = new TextEncoder();

const JSCallbackMount = (module) => {
  const CPP2JSFuncs = {};

  module.runJSFunc = (funcName, strData, seq) => {
    const data = JSON.parse(strData);
    const func = CPP2JSFuncs[funcName];
    if (func) {
      Promise.resolve(func(data))
        .then((buffer) => module.runCPPCallbackByAllocBuffer(0, buffer, seq))
        .catch((err) => {
          console.error(err);
          module.runCPPCallbackByAllocBuffer(1, new Uint8Array(), seq);
        });
    }
  };

  CPP2JSFuncs.loadFontMeta = (data) => {
    return Promise.all([queryLocalFontMeta(), queryGoogleFontMeta()])
      .then(([localFontMeta, googleFontMeta]) => ({
        ...localFontMeta,
        ...googleFontMeta,
      }))
      .then((fontMeta) => {
        return encoder.encode(JSON.stringify(fontMeta));
      });
  };

  CPP2JSFuncs.loadFont = (data) => {
    return fetch(data.path, {
      method: "GET",
      mode: "cors",
    })
      .then((resp) => resp.arrayBuffer())
      .then((buffer) => {
        // throw new Error("test");
        return new Uint8Array(buffer);
      });
  };

  module.afterJSCallbackMounted();
};

Module().then((module) => {
  JSCallbackMount(module);

  // const renderer = module.Renderer.getInstance();
  // renderer.init("#app", appElem.width, appElem.height);
  // renderer.render();

  module.startRender("#app", appElem.width, appElem.height);

  window.layout = () => module.layout();
});
