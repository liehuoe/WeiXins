import { ApiBase, Get } from "./api_base";

class Api extends ApiBase {
  @Get("获取版本号") async version(): Promise<string> {
    return await this.send();
  }
}
export const app = Api.prototype;
