import WebSocket from "ws";

export interface EditorResult {
  ok: boolean;
  error?: string;
  data?: Record<string, unknown>;
}

interface JsonRpcResponse {
  jsonrpc: string;
  id: number;
  result?: EditorResult;
  error?: { code: number; message: string };
}

let requestId = 1;

export class ShotcutClient {
  private ws: WebSocket | null = null;
  private readonly url: string;

  constructor(url = process.env.SHOTCUT_WS_URL ?? "ws://127.0.0.1:3847") {
    this.url = url;
  }

  async connect(): Promise<void> {
    if (this.ws?.readyState === WebSocket.OPEN) {
      return;
    }

    await new Promise<void>((resolve, reject) => {
      const ws = new WebSocket(this.url);
      ws.once("open", () => {
        this.ws = ws;
        resolve();
      });
      ws.once("error", (error) => {
        reject(error);
      });
    });
  }

  async request(method: string, params: Record<string, unknown> = {}): Promise<EditorResult> {
    await this.connect();
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      throw new Error("Shotcut control WebSocket is not connected");
    }

    const id = requestId++;
    const payload = JSON.stringify({ jsonrpc: "2.0", id, method, params });

    return new Promise<EditorResult>((resolve, reject) => {
      const ws = this.ws;
      if (!ws) {
        reject(new Error("Shotcut control WebSocket is not connected"));
        return;
      }

      const onMessage = (data: WebSocket.RawData) => {
        ws.off("message", onMessage);
        ws.off("error", onError);
        try {
          const response = JSON.parse(data.toString()) as JsonRpcResponse;
          if (response.error) {
            reject(new Error(response.error.message));
            return;
          }
          resolve(response.result ?? { ok: false, error: "missing result" });
        } catch (error) {
          reject(error);
        }
      };

      const onError = (error: Error) => {
        ws.off("message", onMessage);
        ws.off("error", onError);
        reject(error);
      };

      ws.on("message", onMessage);
      ws.on("error", onError);
      ws.send(payload);
    });
  }
}
