import { createSignal, For, onMount, Show } from "solid-js";
import { Transition, TransitionGroup } from "solid-transition-group";
import { EmptyText } from "./empty_text";
import { Icon } from "./icon";
import { EnableFocusList } from "./focus_list";
import { Data, Status, type UserData } from "./data";
import { ContextMenu, MenuItem, menu } from "./ctx_menu";
import { Tips } from "./tips";
import { About } from "./about";
import { api } from "../api";

export default function Login() {
  const data = new Data();
  const [selUser, setSelUser] = createSignal<UserData | null>(null);
  onMount(() => data.refresh());
  EnableFocusList();

  // 列表项
  function Item(props: {
    class?: string;
    children?: any;
    ref?: (el: HTMLElement) => void;
    onClick?: (e: MouseEvent) => void;
    onContextMenu?: (e: MouseEvent) => void;
    draggable?: boolean;
    onDragStart?: (e: DragEvent) => void;
    onDragOver?: (e: DragEvent) => void;
    onDrop?: (e: DragEvent) => void;
    onDragEnd?: (e: DragEvent) => void;
  }) {
    return (
      <button
        ref={props.ref}
        class={`transition-all w-full border-none flex items-center border-rd-2 p-1 gap-1 ${props.class}`}
        onclick={props.onClick}
        oncontextmenu={props.onContextMenu}
        draggable={props.draggable}
        onDragStart={props.onDragStart}
        onDragOver={props.onDragOver}
        onDrop={props.onDrop}
        onDragEnd={props.onDragEnd}
      >
        {props.children}
      </button>
    );
  }
  let dragIndex: number | null = null; // 拖拽的索引
  let dragDom: HTMLElement | null = null; // 拖拽添加的元素
  function ReadItem(props: { user: UserData; index: number }) {
    function OnDragStart(e: DragEvent) {
      dragIndex = props.index;
      if (e.dataTransfer) {
        e.dataTransfer.effectAllowed = "move";
      }
    }
    function OnDragEnd() {
      dragIndex = null;
      if (dragDom) {
        dragDom.remove();
        dragDom = null;
      }
    }
    function OnDragDrop(e: DragEvent) {
      e.preventDefault();
      const fromIndex = dragIndex;
      const toIndex = props.index;
      if (fromIndex === null || fromIndex === toIndex) {
        return;
      }
      data.move(fromIndex, toIndex);

      OnDragEnd();
    }
    function OnDragOver(e: DragEvent) {
      e.preventDefault();
      e.stopPropagation();
      if (e.dataTransfer) {
        e.dataTransfer.dropEffect = "move";
      }
      if (dragIndex === null || !e.target) {
        return;
      }

      dragDom?.remove();
      if (dragIndex === props.index) {
        return;
      }
      const dom = e.target as HTMLElement;
      dragDom = document.createElement("div");
      dragDom.className = "h-10 bg-green/50 border-rd-2";
      dragDom.ondragover = (e) => {
        e.preventDefault();
        e.stopPropagation();
        if (e.dataTransfer) {
          e.dataTransfer.effectAllowed = "move";
        }
      };
      dragDom.ondrop = OnDragDrop;
      if (props.index < dragIndex) {
        dom.before(dragDom);
      } else {
        dom.after(dragDom);
      }
    }

    return (
      <Item
        ref={(el) => data.userDoms.push(el)}
        class={`${props.user.login ? "bg-yellow-300" : "bg-green"}
          shadow-dark shadow-2xl hover:shadow-lg hover:bg-op-70 cursor-pointer dark:shadow-black`}
        onClick={() => data.login(props.user, props.index)}
        onContextMenu={(e) => {
          e.preventDefault();
          e.stopImmediatePropagation();
          setSelUser(props.user);
          menu.show(e);
        }}
        draggable
        onDragStart={OnDragStart}
        onDragEnd={OnDragEnd}
        onDragOver={OnDragOver}
        onDrop={OnDragDrop}
      >
        <Show when={data.isMulti}>
          <div
            class={`text-lg pointer-events-none
                ${data.selected[props.index] ? "i-mdi-checkbox-marked-outline" : "i-mdi-checkbox-blank-outline"}`}
          ></div>
        </Show>
        <img
          class="w-10 h-10 border-rd-2 bg-white pointer-events-none"
          style="border: solid #fff 1px"
          src={`/${props.user.dir}.head_img`}
        />
        <div class="items-center flex-1 overflow-hidden pointer-events-none">
          {props.user.name}
        </div>
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
  function isScroll(e: MouseEvent) {
    const dom = e.currentTarget as HTMLElement;
    const rect = dom.getBoundingClientRect();
    const x = e.clientX;
    const y = e.clientY;
    // 计算是否有垂直/水平滚动条
    const hasVerticalScrollbar = dom.scrollHeight > dom.clientHeight;
    const hasHorizontalScrollbar = dom.scrollWidth > dom.clientWidth;
    // 获取滚动条宽度（兼容各浏览器）
    const scrollbarWidth = dom.offsetWidth - dom.clientWidth;
    const scrollbarHeight = dom.offsetHeight - dom.clientHeight;

    const onVerticalScrollbar =
      hasVerticalScrollbar &&
      x >= rect.right - scrollbarWidth &&
      x <= rect.right &&
      y >= rect.top &&
      y <= rect.bottom;

    const onHorizontalScrollbar =
      hasHorizontalScrollbar &&
      y >= rect.bottom - scrollbarHeight &&
      y <= rect.bottom &&
      x >= rect.left &&
      x <= rect.right;

    return onVerticalScrollbar || onHorizontalScrollbar;
  }
  async function onDragWindow(e: MouseEvent) {
    if (e.target !== e.currentTarget) {
      return; // 子元素不触发
    }
    if (e.button !== 0) {
      return; // 只有左键才触发
    }
    if (isScroll(e)) {
      return; // 滚动条不触发
    }

    if (menu.isVisible()) {
      return; // 显示菜单不触发
    }
    await api.app.drag();
  }
  return (
    <>
      <div
        class="z-2 p-1 gap-1 flex-col h-full overflow-y-auto"
        oncontextmenu={() => setSelUser(null)}
        onmousedown={onDragWindow}
      >
        <TransitionGroup
          onEnter={(el, done) => {
            const a = el.animate([{ opacity: 0 }, { opacity: 1 }], {
              duration: 300,
            });
            a.finished.then(done);
          }}
          onExit={(_, done) => done()}
        >
          <For each={data.users}>
            {(user, index) => <ReadItem user={user} index={index()} />}
          </For>
        </TransitionGroup>
      </div>
      <Show when={!data.users?.length}>
        <EmptyText>
          {data.users ? "单击右键添加账号" : "正在加载数据"}
        </EmptyText>
      </Show>
      <Show when={data.isMulti}>
        <div class="p-1">
          <Item
            class={`${data.selected.some((v, i) => v && !data.users![i].login) ? "bg-blue" : "bg-blue-1"} h-10 shadow-dark shadow-2xl hover:shadow-lg hover:bg-op-70 cursor-pointer dark:shadow-black flex justify-center`}
            onClick={() => data.multiLogin()}
          >
            <strong class="text-lg">一键登录</strong>
          </Item>
        </div>
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
          onClick={() => {
            setSelUser(null);
            data.status = Status.Edit;
          }}
        >
          添加账号
        </MenuItem>
        <MenuItem
          icon={
            data.isMulti
              ? "i-mdi-checkbox-marked-outline bg-green"
              : "i-mdi-checkbox-blank-outline bg-red"
          }
          onClick={() => (data.isMulti = !data.isMulti)}
        >
          多选模式
        </MenuItem>
        <MenuItem
          icon="i-mdi-about-outline"
          onClick={() => (data.status = Status.About)}
        >
          支持作者
        </MenuItem>
      </ContextMenu>
      <Tips class={data.isMulti ? "bottom-12" : "bottom-2"} />
      <About
        when={data.status === Status.About}
        onClose={() => (data.status = Status.Normal)}
      />
    </>
  );
}
