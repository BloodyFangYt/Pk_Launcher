import { describe, expect, it } from "vitest";

import { formatBytes } from "./format-bytes";

describe("formatBytes", () => {
  it("formats bytes to a human readable string", () => {
    expect(formatBytes(0)).toBe("0 B");
    expect(formatBytes(1024)).toBe("1 KB");
    expect(formatBytes(5 * 1024 * 1024)).toBe("5 MB");
    expect(formatBytes(3.5 * 1024 * 1024)).toBe("3.5 MB");
    expect(formatBytes(10 * 1024 * 1024 * 1024)).toBe("10 GB");
  });

  it("rounds large values to whole numbers", () => {
    expect(formatBytes(12 * 1024)).toBe("12 KB");
  });

  it("throws on invalid input", () => {
    expect(() => formatBytes(-1)).toThrow();
    expect(() => formatBytes(Number.NaN)).toThrow();
  });
});