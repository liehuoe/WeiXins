import { createSignal, onCleanup, onMount } from "solid-js";
import { api } from "../api";

export { SystemBtns, SystemBtn, isFocus };

const [isFocus, setIsFocus] = createSignal(true);
function SystemBtn(props: {
  icon: string;
  class?: string;
  onclick?: (e: MouseEvent) => void;
  title?: string;
}) {
  return (
    <div
      class={`h-8 w-8 flex-center hover:bg-fore/10
        ${isFocus() || "text-fore/50"} ${props.class}`}
      onclick={props.onclick}
      title={props.title}
    >
      <div class={`size-4 ${props.icon}`}></div>
    </div>
  );
}

function SystemBtns() {
  const [isMaximize, setIsMaximize] = createSignal(false);

  onMount(() => {
    window.addEventListener("focus", () => setIsFocus(true));
    window.addEventListener("blur", () => setIsFocus(false));
    window.onMaximize = (maximize) => setIsMaximize(maximize);
  });
  onCleanup(() => {
    window.removeEventListener("focus", () => setIsFocus(true));
    window.removeEventListener("blur", () => setIsFocus(false));
  });
  return (
    <>
      <SystemBtn
        icon="i-mdi-minimize"
        onclick={() => api.app.minimize()}
        title="最小化"
      />
      <SystemBtn
        icon={`${isMaximize() ? "i-mdi-window-restore" : "i-mdi-maximize"}`}
        onclick={() => api.app.maximize()}
        title={`${isMaximize() ? "还原" : "最大化"}`}
      />
      <SystemBtn
        icon="i-mdi-close"
        class="hover:bg-red"
        onclick={() => api.app.close()}
        title="关闭"
      />
    </>
  );
}
