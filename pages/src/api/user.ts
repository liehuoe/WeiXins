import { ApiBase, Get, Set } from "./api_base";

export interface User {
  name: string; // 微信昵称
  dir: string; // 账号保存目录
}

class Api extends ApiBase implements User {
  name = "";
  dir = "";

  @Get("获取账号列表") async get(): Promise<{
    user: User[];
    isMulti?: Boolean;
    selected?: number[];
  }> {
    const data = await this.send();
    if (Array.isArray(data)) {
      // v2.0.0 以前(不含)
      return { user: data, isMulti: false, selected: [] };
    } else {
      return data;
    }
  }
  @Get("获取已经登录的账号") async getDirs(): Promise<User["dir"][]> {
    return await this.send();
  }
  @Set("登录账号") async login(arg: Pick<User, "dir">[]): Promise<void> {
    return await this.send(arg.map((item) => ({ dir: item.dir })));
  }
  @Set("添加或修改账号信息") async set(
    arg: User | Pick<User, "dir">,
  ): Promise<void> {
    return await this.send(arg);
  }
  @Set("修改多选模式参数") async setMulti(arg: {
    isMulti?: Boolean;
    selected?: number[];
  }): Promise<void> {
    return await this.send(arg);
  }
}
export const user = Api.prototype;
