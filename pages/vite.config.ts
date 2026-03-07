import { defineConfig } from "vite";
import unocss from "unocss/vite";
import solid from "vite-plugin-solid";
import { viteSingleFile } from "vite-plugin-singlefile";

export default defineConfig({
  plugins: [unocss(), solid(), viteSingleFile()],
  build: {
    modulePreload: false,
    minify: "terser",
  },
});
