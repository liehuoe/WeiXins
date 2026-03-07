/* @refresh reload */
import "virtual:uno.css";
import "./index.css";
import { render } from "solid-js/web";
import { lazy } from "solid-js";

// 禁止拖拽文件到窗口
document.addEventListener("dragover", (e) => e.preventDefault());
document.addEventListener("drop", (e) => e.preventDefault());

const route = {
  login: lazy(() => import("./login")),
};

const dom = document.getElementById("app")!;
const name = new URLSearchParams(window.location.search).get("page");
if (name && name in route) {
  render(route[name as keyof typeof route], dom);
} else {
  render(route.login, dom);
}
