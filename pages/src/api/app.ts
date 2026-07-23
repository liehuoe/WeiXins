import { ApiBase, Get, Set } from "./api_base";

declare global {
  interface Window {
    /** C++ 通知js窗口的最大化状态 */
    onMaximize: (maximize: boolean) => void;
  }
}

class Api extends ApiBase {
  @Get("获取版本号") async version(): Promise<string> {
    return await this.send();
  }
  @Set("关闭窗口") async close(): Promise<void> {
    return await this.send();
  }
  @Set("最大化/还原窗口") async maximize(): Promise<void> {
    return await this.send();
  }
  @Set("最小化窗口") async minimize(): Promise<void> {
    return await this.send();
  }
  @Set("拖动窗口") async drag(): Promise<void> {
    return await this.send();
  }
  @Set("置顶窗口") async pin(arg: { pin?: boolean }): Promise<boolean> {
    return await this.send(arg);
  }
}
export const app = Api.prototype;
