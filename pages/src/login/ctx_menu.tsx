export { ContextMenu, MenuItem, menu };
import { createSignal, onCleanup, onMount } from "solid-js";

interface Point {
  x: number;
  y: number;
}
const [menuPos, setMenuPos] = createSignal<Point | null>(null);
let menuDom: HTMLElement;

const menu = {
  show(e: MouseEvent) {
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
  },
  isVisible() {
    return menuPos() !== null;
  },
};

function onClickOutside() {
  setMenuPos(null);
}
function ContextMenu(props: { when?: boolean; children: any }) {
  onMount(() => {
    document.addEventListener("click", onClickOutside);
  });
  onCleanup(() => {
    document.removeEventListener("click", onClickOutside);
  });
  return (
    <div
      ref={(el) => (menuDom = el)}
      class="z-102 border-rd-2 py-1 bg-back fixed shadow-black shadow-lg flex-col"
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
      class="gap-1 px-2 py-1 hover:bg-fore/10 whitespace-nowrap items-center"
      onclick={props.onClick}
    >
      <div class={`text-lg ${props.icon}`} />
      <div>{props.children}</div>
    </div>
  );
}
