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
      class="z-102 border-rd-2 py-1 bg-gray-100 fixed shadow-dark shadow-lg flex-col
      dark:shadow-black dark:bg-dark-100 dark:color-light"
      style={{
        left: (menuPos()?.x || 0) + "px",
        top: (menuPos()?.y || 0) + "px",
        visibility:
          menuPos() && (props.when === undefined || props.when)
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
      class="gap-1 px-2 py-1 hover:bg-gray-300 whitespace-nowrap items-center
      dark:hover:bg-dark-300"
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
