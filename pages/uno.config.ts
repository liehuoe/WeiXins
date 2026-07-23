import { defineConfig, presetIcons, presetWind3 } from "unocss";

export default defineConfig({
  presets: [presetWind3({ dark: "media" }), presetIcons()],
  shortcuts: {
    "flex-center": "items-center justify-center",
  },
  theme: {
    colors: {
      fore: "rgb(var(--fore-color))",
      back: "rgb(var(--back-color))",
    },
  },
  safelist: ["i-mdi-error", "i-mdi-info"], // showTip 动态使用
});
