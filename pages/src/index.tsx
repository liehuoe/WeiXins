/* @refresh reload */
import "virtual:uno.css";
import "./index.css";
import { render } from "solid-js/web";
import login from "./login";

// 禁止拖拽文件到窗口
document.addEventListener("dragover", (e) => {
  e.preventDefault();
  if (e.dataTransfer) {
    e.dataTransfer.dropEffect = "none";
  }
});
document.addEventListener("contextmenu", (e) => e.preventDefault());

const route = { login };

const dom = document.getElementById("app")!;
const name = new URLSearchParams(window.location.search).get("page");
if (name && name in route) {
  render(route[name as keyof typeof route], dom);
} else {
  render(route.login, dom);
}
