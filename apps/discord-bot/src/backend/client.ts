import ky, { HTTPError, type KyInstance, type Options } from "ky";

/**
 * Base URL of the central PK Launcher backend. The bot is stateless: it never
 * talks to a database directly and instead calls the backend's /bot contract.
 * Override with BOT_API_URL in the environment.
 */
export const DEFAULT_BOT_API_URL =
  process.env.BOT_API_URL ?? "http://localhost:8080";

/** Shape returned by the backend for a Discord-linked account. */
export interface BotAccount {
  discordId: string;
  username: string | null;
  linked: boolean;
  linkedAt: string | null;
  balance: number;
  premium: boolean;
  role: string | null;
}

/** Payload for POST /api/v1/bot/link (one-time launcher code). */
export interface LinkPayload {
  discordId: string;
  code: string;
}

/** Typed, unified error for failed backend calls. */
export class ApiError extends Error {
  readonly status?: number;

  constructor(message: string, status?: number) {
    super(message);
    this.name = "ApiError";
    this.status = status;
  }
}

/**
 * Thin ky wrapper for the backend /bot contract:
 *   - POST /api/v1/bot/link  -> link a Discord account with a launcher code
 *   - GET  /api/v1/bot/me    -> account profile (balance/status/premium)
 */
export class BotApiClient {
  private readonly http: KyInstance;

  constructor(baseUrl: string = DEFAULT_BOT_API_URL, options: Options = {}) {
    this.http = ky.create({
      prefixUrl: `${baseUrl.replace(/\/+$/, "")}/api/v1/bot/`,
      timeout: 10_000,
      retry: 1,
      ...options,
    });
  }

  async link(payload: LinkPayload): Promise<BotAccount> {
    return this.#request("POST", "link", { json: payload });
  }

  async me(discordId: string): Promise<BotAccount> {
    return this.#request("GET", "me", { searchParams: { discordId } });
  }

  async #request<T>(
    method: "GET" | "POST",
    path: string,
    options: { json?: LinkPayload; searchParams?: Record<string, string> },
  ): Promise<T> {
    try {
      const res =
        method === "POST"
          ? await this.http.post(path, { json: options.json })
          : await this.http.get(path, { searchParams: options.searchParams });
      return (await res.json()) as T;
    } catch (error) {
      throw this.#toApiError(error);
    }
  }

  #toApiError(error: unknown): ApiError {
    if (error instanceof ApiError) return error;
    if (error instanceof HTTPError) {
      return new ApiError(
        `Backend request failed (HTTP ${error.response.status}).`,
        error.response.status,
      );
    }
    const message = error instanceof Error ? error.message : String(error);
    return new ApiError(message);
  }
}