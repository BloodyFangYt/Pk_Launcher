export function formatBytes(bytes: number): string {
  if (bytes < 0 || !Number.isFinite(bytes)) {
    throw new Error("bytes must be a non-negative finite number");
  }
  if (bytes === 0) {
    return "0 B";
  }
  const units = ["B", "KB", "MB", "GB", "TB"] as const;
  const index = Math.min(
    Math.floor(Math.log(bytes) / Math.log(1024)),
    units.length - 1,
  );
  const value = bytes / 1024 ** index;
  const rounded = Number.isInteger(value) ? String(value) : value.toFixed(1);
  return `${rounded} ${units[index]}`;
}