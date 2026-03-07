import { ApiBase, Get, Set } from "./api_base";

export interface User {
  name: string; // 微信昵称
  dir: string; // 账号保存目录
}

class Api extends ApiBase implements User {
  name = "";
  dir = "";

  @Get("获取账号列表") async get(): Promise<User[]> {
    return await this.send();
  }
  @Set("登录账号") async login(arg: Pick<User, "dir">): Promise<void> {
    return await this.send({ dir: arg.dir });
  }
  @Set("修改账号列表") async set(arg: User[]): Promise<void> {
    return await this.send(arg);
  }
}
export const user = Api.prototype;
