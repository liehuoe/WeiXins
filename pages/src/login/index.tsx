import { createSignal, For, onMount, Show } from "solid-js";
import { Transition, TransitionGroup } from "solid-transition-group";
import { EmptyText } from "./empty_text";
import { Icon } from "./icon";
import { EnableFocusList } from "./focus_list";
import { Data, Status } from "./data";
import type { User } from "../api/user";
import { ContextMenu, MenuItem, showContextMenu } from "./ctx_menu";
import { Tips } from "./tips";

export default function Login() {
  const data = new Data();
  onMount(() => data.refresh());
  EnableFocusList();

  // 列表项
  function Item(props: {
    class?: string;
    children?: any;
    ref?: (el: HTMLElement) => void;
    onClick?: (e: MouseEvent) => void;
    onContextMenu?: (e: MouseEvent) => void;
  }) {
    return (
      <div class="w-full">
        <button
          ref={props.ref}
          class={`w-full border-none flex items-center border-rd-2 p-1 gap-1 ${props.class}`}
          onclick={props.onClick}
          oncontextmenu={props.onContextMenu}
        >
          {props.children}
        </button>
      </div>
    );
  }
  const [selUser, setSelUser] = createSignal<User | null>(null);
  function ReadItem(props: { user: User }) {
    return (
      <Item
        ref={(el) => data.userDoms.push(el)}
        class="bg-green shadow-dark shadow-2xl hover:bg-op-70 cursor-pointer"
        onClick={() => data.login(props.user)}
        onContextMenu={(e) => {
          e.preventDefault();
          e.stopImmediatePropagation();
          setSelUser(props.user);
          showContextMenu(e);
        }}
      >
        <img
          class="w-10 h-10 border-rd-2 bg-white"
          style="border: solid #fff 1px"
          src={`/${props.user.dir}.head_img`}
        />
        <div class="items-center flex-1">{props.user.name}</div>
      </Item>
    );
  }
  let inputDom: HTMLInputElement;
  async function onSave() {
    const user = selUser();
    if (user) {
      await data.modify({ user, newName: inputDom.value });
    } else {
      await data.add({ name: inputDom.value });
    }
    data.status = Status.Normal;
  }
  function EditItem() {
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
        <Show when={data.status === Status.Edit}>
          <div class="z-100 bg-black/80 fixed inset-0 p-1 items-center">
            <Item class="shadow-dark bg-blue shadow-xl">
              <img
                class="w-10 h-10 border-rd-2 bg-white"
                style="border: solid #fff 1px"
                src={`/${selUser()?.dir}.head_img`}
              />
              <input
                ref={(el) => {
                  inputDom = el;
                  data.editDoms.push(el);
                  setTimeout(() => el.focus(), 0); // 默认焦点
                }}
                type="text"
                placeholder="请输入账号名称"
                class="w-full p-2 border-none outline-none border-rd-2 transition duration-300 focus:shadow-md"
                value={selUser()?.name || ""}
              />
              <Icon
                ref={(el) => data.editDoms.push(el)}
                title="确定"
                icon="i-mdi-check"
                class="text-light bg-green"
                onClick={() => onSave()}
              />
              <Icon
                ref={(el) => data.editDoms.push(el)}
                title="取消"
                icon="i-mdi-close"
                class="text-light bg-red"
                onClick={() => (data.status = Status.Normal)}
              />
            </Item>
          </div>
        </Show>
      </Transition>
    );
  }
  function About() {
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
        <Show when={data.status === Status.About}>
          <div
            class="z-100 bg-black/80 fixed inset-0 flex-col items-center justify-center text-white"
            onclick={() => (data.status = Status.Normal)}
          >
            <div>打赏为我续命</div>
            <img class="max-w-1/2 max-h-1/3" src="/1002.jpg" />
            <p />
            <div>商业合作请加微信</div>
            <img class="max-w-1/2 max-h-1/3" src="/1001.jpg" />
            <div>程序版本：1.0.0</div>
          </div>
        </Show>
      </Transition>
    );
  }
  return (
    <div
      class="h-full p-1 gap-1 flex-col"
      oncontextmenu={() => setSelUser(null)}
    >
      <Show
        when={data.users?.length}
        fallback={
          <EmptyText>
            {data.users ? "单击右键添加账号" : "正在加载数据"}
          </EmptyText>
        }
      >
        <TransitionGroup
          onEnter={(el, done) => {
            const a = el.animate(
              [
                { opacity: 0, transform: "translateY(-100%)" },
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
                { opacity: 0, transform: "translateY(-100%)" },
              ],
              {
                duration: 300,
                easing: "ease-in-out",
              },
            );
            a.finished.then(done);
          }}
        >
          <For each={data.users}>{(user) => <ReadItem user={user} />}</For>
        </TransitionGroup>
      </Show>
      <EditItem />
      <ContextMenu when={data.status === Status.Normal}>
        <Show when={selUser()}>
          <MenuItem
            icon="i-mdi-edit bg-orange"
            onClick={() => (data.status = Status.Edit)}
          >
            修改名称
          </MenuItem>
          <MenuItem
            icon="i-mdi-delete-outline bg-red"
            onClick={() => data.modify({ user: selUser()! })}
          >
            删除账号
          </MenuItem>
        </Show>
        <MenuItem icon="i-mdi-refresh bg-green" onClick={() => data.refresh()}>
          刷新列表
        </MenuItem>
        <MenuItem
          icon="i-mdi-add bg-blue"
          onClick={() => (data.status = Status.Edit)}
        >
          添加账号
        </MenuItem>
        <MenuItem
          icon="i-mdi-about-outline"
          onClick={() => (data.status = Status.About)}
        >
          支持作者
        </MenuItem>
      </ContextMenu>
      <Tips />
      <About />
    </div>
  );
}
