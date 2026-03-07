export interface ApiOptions {
  /** 超时时间, 默认10秒 */
  timeout?: number;
}
export class ApiBase {
  send!: (arg?: unknown, opts?: ApiOptions) => Promise<any>;
}

export function Get(_name: string) {
  return function (_target: unknown, _method: string) {
    // 暂无用途
  };
}
export function Set(_name: string) {
  return function (_target: unknown, _method: string) {
    // 暂无用途
  };
}

/** webview实现 */
declare global {
  interface Window {
    SendCppMsg(message: unknown): void;
    SetCppMsgHandler(listener: (data: unknown) => void): void;
  }
}

interface Result {
  code: number;
  error?: string;
  data: unknown;
}
class WebView {
  constructor() {
    window.SetCppMsgHandler((data: unknown) => {
      const ctx = data as Result & { id: number };
      const callback = this.eventMap.get(ctx.id);
      if (!callback) {
        return;
      }
      callback(ctx);
    });
  }
  public async send(
    url: string,
    data: unknown,
    opts?: ApiOptions
  ): Promise<any> {
    const ctx = {
      id: this.getFlowId(),
      url,
      data,
    };
    window.SendCppMsg(ctx); // 发送请求
    return new Promise((resolve, reject) => {
      // 设置超时
      const timer = setTimeout(() => {
        this.eventMap.delete(ctx.id);
        reject(new Error("timeout"));
      }, opts?.timeout || 10 * 1000);
      // 订阅响应事件
      this.eventMap.set(ctx.id, (result) => {
        clearTimeout(timer);
        if (result.code === 0) {
          resolve(result.data);
        } else {
          reject(new Error(result.error));
        }
      });
    });
  }

  private getFlowId() {
    const id = ++this.flowId;
    if (id >= 1000000) {
      this.flowId = 0;
    }
    return id;
  }
  private flowId = 0;
  private eventMap = new Map<number, (result: Result) => void>();
}

class NullSender {
  public async send(
    _url: string,
    _data: unknown,
    _opts?: ApiOptions
  ): Promise<any> {
    throw new Error("Not implemented");
  }
}

const sender = "SendCppMsg" in window ? new WebView() : new NullSender();
export function initApi(url: string, obj: Record<string, unknown>) {
  for (const key of Object.getOwnPropertyNames(obj)) {
    const item = obj[key];
    if (typeof item === "object") {
      initApi(`${url}/${key}`, item as Record<string, unknown>);
    } else if (typeof item === "function" && key !== "constructor") {
      obj[key] = async (...args: unknown[]) => {
        obj.send = (arg: unknown) => sender.send(`${url}/${key}`, arg);
        return await item.call(obj, ...args);
      };
    }
  }
}
