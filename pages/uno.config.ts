import { defineConfig, presetIcons, presetWind3 } from "unocss";

export default defineConfig({
  presets: [presetWind3({ dark: "media" }), presetIcons()],
  safelist: ["i-mdi-error", "i-mdi-warning"], // showTip 动态使用
});
