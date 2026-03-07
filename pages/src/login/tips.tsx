export { Tips, type Tip, showTip };
import { createSignal, For, Show } from "solid-js";
import { TransitionGroup } from "solid-transition-group";

interface Tip {
  msg: string;
  icon?: string;
}
const [tips, setTips] = createSignal<Tip[]>([]);

function Tips() {
  return (
    <div class="z-101 gap-1 fixed bottom-2 left-1 right-1 flex-col items-center">
      <TransitionGroup
        onEnter={(el, done) => {
          const a = el.animate(
            [
              { opacity: 0, transform: "translateY(100%)" },
              { opacity: 1, transform: "translateY(0)" },
            ],
            {
              duration: 300,
              easing: "ease-in-out",
            },
          );
          a.finished.then(done);
        }}
        onExit={(el, done) => {
          const a = el.animate(
            [
              { opacity: 1, transform: "translateY(0)" },
              { opacity: 0, transform: "translateY(100%)" },
            ],
            {
              duration: 300,
              easing: "ease-in-out",
            },
          );
          a.finished.then(done);
        }}
      >
        <For each={tips()}>
          {(tip) => (
            <div class="gap-1 px-2 py-1 border-rd-2 shadow-dark shadow-md bg-gray-100 items-center">
              <Show when={tip.icon}>
                <div class={`text-lg ${tip.icon}`} />
              </Show>
              <div class="break-all">{tip.msg}</div>
            </div>
          )}
        </For>
      </TransitionGroup>
    </div>
  );
}
let timer: number = 0;
function showTip(tip: Tip) {
  setTips([tip, ...tips()]);
  if (timer) {
    return;
  }
  timer = setInterval(() => {
    const newTips = tips().slice(0, -1);
    setTips(newTips);
    if (newTips.length === 0) {
      clearInterval(timer);
      timer = 0;
    }
  }, 5000);
}
