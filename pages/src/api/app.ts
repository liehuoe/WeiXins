import { ApiBase, Get, Set } from "./api_base";

class Api extends ApiBase {
  @Get("获取版本号") async version(): Promise<string> {
    return await this.send();
  }
  @Set("拖动窗口") async drag(): Promise<void> {
    return await this.send();
  }
}
export const app = Api.prototype;
