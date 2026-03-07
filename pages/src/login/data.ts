import { createSignal } from "solid-js";
import { api } from "../api";
import type { User } from "../api/user";
import { SetFocusList } from "./focus_list";
import { showTip } from "./tips";

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
      const newUsers = await api.user.get();
      // 相同的账号信息使用旧的引用(为了动画效果)
      const oldUsers = this.users || [];
      this.users = newUsers.map((newUser) => {
        const oldUser = oldUsers.find(
          (user) => newUser.dir === user.dir && newUser.name === user.name,
        );
        return oldUser || newUser;
      });
    } catch {
      this.users = [];
    }
  }
  /** 登录账号 */
  async login(user: User) {
    await api.user.login(user);
  }
  /** 添加账号 */
  async add(arg: Pick<User, "name">) {
    if (!arg.name) {
      showTip({ msg: "账号名称不能为空", icon: "i-mdi-warning bg-orange" });
      throw new Error("账号名称不能为空");
    }
    const now = new Date();
    let dir = "";
    dir += now.getFullYear();
    dir += String(now.getMonth() + 1).padStart(2, "0");
    dir += String(now.getHours()).padStart(2, "0");
    dir += String(now.getMinutes()).padStart(2, "0");
    dir += String(now.getSeconds()).padStart(2, "0");
    this.users = [...this.users!, { name: arg.name, dir }];
    try {
      await api.user.set(this.users);
    } catch {
      showTip({ msg: "添加账号出错", icon: "i-mdi-error bg-red" });
      throw new Error("添加账号出错");
    }
  }
  async modify(arg: { user: User; newName?: string }) {
    const users = this.users;
    if (!users) return;
    const index = users.indexOf(arg.user);
    const newUsers = [...users.slice(0, index)];
    let action = "删除";
    if (arg.newName) {
      action = "修改";
      newUsers.push({ name: arg.newName, dir: arg.user.dir });
    } else if (arg.newName === "") {
      showTip({ msg: "账号名称不能为空", icon: "i-mdi-warning bg-orange" });
      throw new Error("账号名称不能为空");
    }
    newUsers.push(...users.slice(index + 1));
    this.users = newUsers;
    try {
      await api.user.set(this.users!);
    } catch {
      showTip({ msg: action + "账号出错", icon: "i-mdi-error bg-red" });
      throw new Error(action + "账号出错");
    }
  }
  /** 账号数据 */
  get users() {
    return this._users[0]();
  }
  set users(value: User[] | null) {
    this._users[1](value);
  }
  private _users = createSignal<User[] | null>(null);
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
}
