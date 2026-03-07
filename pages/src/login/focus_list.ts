export { SetFocusList, EnableFocusList };
import { onCleanup, onMount } from "solid-js";

let focusList: HTMLElement[] = [];
// 循环移动列表的焦点
function focus(btn: EventTarget | null, offset: number) {
  let pos = focusList.indexOf(btn as HTMLElement);
  if (pos === -1) {
    if (focusList.length > 0) {
      focusList[0].focus();
    }
    return;
  }
  pos += offset;
  if (pos < 0) {
    pos = focusList.length - 1;
  }
  if (pos >= focusList.length) {
    pos = 0;
  }
  focusList[pos].focus();
}
function onKeyDown(e: KeyboardEvent) {
  if (focusList.length === 0) {
    return;
  }
  let offset = 0;
  if (e.key === "ArrowUp") {
    offset = -1;
  } else if (e.key === "ArrowDown") {
    offset = 1;
  } else if (e.key === "Tab") {
    offset = e.shiftKey ? -1 : 1;
  }
  if (offset !== 0) {
    e.preventDefault();
    focus(e.target, offset);
  }
}
function EnableFocusList() {
  onMount(() => document.addEventListener("keydown", onKeyDown));
  onCleanup(() => document.removeEventListener("keydown", onKeyDown));
}
function SetFocusList(list: HTMLElement[]) {
  focusList = list;
}
