export { ContextMenu, MenuItem, showContextMenu };
import { createSignal, onCleanup, onMount } from "solid-js";

interface Point {
  x: number;
  y: number;
}
const [menuPos, setMenuPos] = createSignal<Point | null>(null);
let menuDom: HTMLElement;

function ContextMenu(props: { when?: boolean; children: any }) {
  onMount(() => {
    document.addEventListener("contextmenu", showContextMenu);
    document.addEventListener("click", onClickOutside);
  });
  onCleanup(() => {
    document.removeEventListener("contextmenu", showContextMenu);
    document.removeEventListener("click", onClickOutside);
  });
  return (
    <div
      ref={(el) => (menuDom = el)}
      class="z-102 border-rd-2 py-1 bg-gray-100 fixed shadow-dark shadow-lg flex-col"
      style={{
        left: menuPos()?.x ? menuPos()!.x + "px" : "-100%",
        top: menuPos()?.y ? menuPos()!.y + "px" : "-100%",
        visibility:
          menuPos() && (props.when === undefined ? true : props.when)
            ? "visible"
            : "hidden",
      }}
    >
      {props.children}
    </div>
  );
}

function MenuItem(props: {
  icon?: string;
  children: string;
  onClick?: (e: MouseEvent) => void;
}) {
  return (
    <div
      class="gap-1 px-2 py-1 hover:bg-gray-300 whitespace-nowrap"
      onclick={props.onClick}
    >
      <div class={`text-lg ${props.icon}`} />
      <div>{props.children}</div>
    </div>
  );
}

function showContextMenu(e: MouseEvent) {
  e.preventDefault();
  let x = 0;
  let y = 0;
  const menu = menuDom.getBoundingClientRect();
  if (window.innerWidth - e.x >= menu.width) {
    x = e.x;
  } else if (e.x >= menu.width) {
    x = e.x - menu.width;
  }
  if (window.innerHeight - e.y >= menu.height) {
    y = e.y;
  } else if (e.y >= menu.height) {
    y = e.y - menu.height;
  }
  setMenuPos({ x, y });
}
function onClickOutside() {
  setMenuPos(null);
}
