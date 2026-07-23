import { createSignal, onMount, Show } from "solid-js";
import { Transition } from "solid-transition-group";
import { api } from "../api";
import { showTip } from "./tips";

async function getLatestVersion() {
  const response = await fetch(
    "https://api.github.com/repos/liehuoe/WeiXins/releases/latest",
  );
  if (!response.ok) {
    throw new Error(`HTTP 错误！状态码: ${response.status}`);
  }
  const data = await response.json();
  return data.tag_name.replace("v", "");
}

export function About(props: { when: boolean; onClose: () => void }) {
  const [version, setVersion] = createSignal<string>("");
  onMount(async () => {
    const cur = await api.app.version();
    setVersion(cur);
    const latest = await getLatestVersion();
    if (latest !== cur) {
      showTip({
        msg: `发现新版本：v${latest}`,
        icon: "i-mdi-info bg-blue",
      });
    }
  });
  return (
    <Transition
      onEnter={(el, done) => {
        const a = el.animate([{ opacity: 0 }, { opacity: 1 }], {
          duration: 300,
        });
        a.finished.then(done);
      }}
      onExit={(el, done) => {
        const a = el.animate([{ opacity: 1 }, { opacity: 0 }], {
          duration: 300,
        });
        a.finished.then(done);
      }}
    >
      <Show when={props.when}>
        <div
          class="z-100 bg-black/80 fixed inset-0 flex-col flex-center text-white"
          onclick={() => props.onClose()}
        >
          <div>打赏作者</div>
          <img class="max-w-1/2 max-h-1/3" src="/1002.jpg" />
          <p />
          <div>商业合作请加微信</div>
          <img class="max-w-1/2 max-h-1/3" src="/1001.jpg" />
          <div>程序版本：{version()}</div>
        </div>
      </Show>
    </Transition>
  );
}
