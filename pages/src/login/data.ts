import { createSignal } from "solid-js";
import { createStore } from "solid-js/store";
import { api } from "../api";
import type { User } from "../api/user";
import { SetFocusList } from "./focus_list";
import { showTip } from "./tips";

export type UserData = User & { login: Boolean };

export const Status = {
  Normal: "Normal",
  Edit: "Edit",
  About: "About",
} as const;
export type Status = (typeof Status)[keyof typeof Status];

export class Data {
  /** 刷新数据 */
  async refresh() {
    this.userDoms = [];
    SetFocusList(this.userDoms);

    try {
      const [data, dirs] = await Promise.all([
        api.user.get(),
        api.user.getDirs(),
      ]);
      // 更新用户数据
      const dirSet = new Set(dirs);
      this.users = data.user.map((user) => ({
        ...user,
        login: dirSet.has(user.dir),
      }));
      // 多选模式
      this.isMulti = Boolean(data.isMulti);
      const selectedSet = new Set(data.selected);
      this.selected = data.user.map((_, i) => selectedSet.has(i));
    } catch {
      this.users = [];
    }
  }
  /** 登录账号 */
  async login(user: UserData, index: number) {
    if (this.isMulti) {
      this.setSelected(index, !this.selected[index]);
      return;
    }
    if (user.login) {
      showTip({ msg: "此账号已打开", icon: "i-mdi-error bg-red" });
    } else {
      await api.user.login([user]);
    }
  }
  /** 批量登录 */
  async multiLogin() {
    const users = this.users!.filter((_, i) => this.selected[i]).filter(
      (user) => !user.login,
    );
    if (users.length === 0) {
      showTip({ msg: "请选择要登录的账号", icon: "i-mdi-error bg-orange" });
      return;
    }
    await api.user.login(users);
  }
  /** 添加账号 */
  async add(arg: Pick<User, "name">) {
    if (!arg.name) {
      showTip({ msg: "账号名称不能为空", icon: "i-mdi-error bg-red" });
      throw new Error("账号名称不能为空");
    }
    const now = new Date();
    let dir = "";
    dir += now.getFullYear();
    dir += String(now.getMonth() + 1).padStart(2, "0");
    dir += String(now.getHours()).padStart(2, "0");
    dir += String(now.getMinutes()).padStart(2, "0");
    dir += String(now.getSeconds()).padStart(2, "0");
    this.users = [...this.users!, { name: arg.name, dir, login: false }];
    this.selected = [...this.selected, false];
    try {
      await api.user.set({ name: arg.name, dir });
    } catch {
      showTip({ msg: "添加账号出错", icon: "i-mdi-error bg-red" });
      throw new Error("添加账号出错");
    }
  }
  async modify(arg: { user: UserData; newName?: string }) {
    const users = this.users;
    if (!users) return;
    const index = users.indexOf(arg.user);
    const newUsers = [...users.slice(0, index)];
    let action = "删除";
    if (arg.newName) {
      action = "修改";
      newUsers.push({ ...arg.user, name: arg.newName });
    } else if (arg.newName === "") {
      showTip({ msg: "账号名称不能为空", icon: "i-mdi-error bg-red" });
      throw new Error("账号名称不能为空");
    } else {
      this.selected = [
        ...this.selected.slice(0, index),
        ...this.selected.slice(index + 1),
      ];
    }
    newUsers.push(...users.slice(index + 1));
    this.users = newUsers;
    try {
      await api.user.set({
        ...(arg.newName ? { name: arg.newName } : {}),
        dir: arg.user.dir,
      });
    } catch {
      showTip({ msg: action + "账号出错", icon: "i-mdi-error bg-red" });
      throw new Error(action + "账号出错");
    }
  }
  /** 账号数据 */
  get users() {
    return this._users[0]();
  }
  set users(value: UserData[] | null) {
    this._users[1](value);
  }
  private _users = createSignal<UserData[] | null>(null);
  /** 当前状态 */
  get status() {
    return this._status[0]();
  }
  set status(value: Status) {
    if (value !== Status.Normal) {
      this.editDoms = [];
      SetFocusList(this.editDoms);
    }
    this._status[1](value);
  }
  private _status = createSignal<Status>(Status.Normal);

  // 账号列表的焦点元素
  userDoms: HTMLElement[] = [];
  // 编辑对话框的焦点元素
  editDoms: HTMLElement[] = [];

  /** 多选模式 */
  private _isMulti = createSignal<Boolean>(false);
  get isMulti() {
    return this._isMulti[0]();
  }
  set isMulti(value: Boolean) {
    if (this.isMulti == value) {
      return;
    }
    api.user.setMulti({ isMulti: value });
    this._isMulti[1](value);
  }
  /** 勾选的下标 */
  private _selected = createStore<Boolean[]>([]);
  get selected() {
    return this._selected[0];
  }
  set selected(value: Boolean[]) {
    this._selected[1](value);
  }
  setSelected(index: number, value: Boolean) {
    this._selected[1](index, value);
    if (this.delayTimer) {
      return;
    }
    // 延迟发送请求，避免短时间内多次修改导致的重复请求
    this.delayTimer = window.setTimeout(() => {
      this.delayTimer = null;
      api.user.setMulti({
        selected: this.selected
          .map((item, i) => (item ? i : -1))
          .filter((i) => i != -1),
      });
    }, 1000);
  }
  delayTimer: number | null = null;
}
