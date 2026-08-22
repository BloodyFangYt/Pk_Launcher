"use client";

import { useState } from "react";

const OS_COMMANDS: Record<string, { label: string; command: string }> = {
  windows: {
    label: "Windows (winget)",
    command: "winget install PkLauncher.PkLauncher",
  },
  macos: {
    label: "macOS (Homebrew)",
    command: "brew install --cask pk-launcher",
  },
  linux: {
    label: "Linux (AppImage)",
    command: "chmod +x pk-launcher.AppImage && ./pk-launcher.AppImage",
  },
};

type OsKey = keyof typeof OS_COMMANDS;

const OS_OPTIONS = Object.keys(OS_COMMANDS) as OsKey[];

export function DownloadCommands() {
  const [active, setActive] = useState<OsKey>("windows");
  const current = OS_COMMANDS[active];

  return (
    <section className="mt-20" aria-labelledby="cli-heading">
      <h2
        id="cli-heading"
        className="text-2xl font-bold text-white md:text-3xl"
      >
        Alternative installs
      </h2>
      <p className="mt-2 text-text-secondary">
        Install via your terminal or package manager.
      </p>

      <div className="mt-6 flex flex-wrap gap-2">
        {OS_OPTIONS.map((os) => (
          <button
            key={os}
            type="button"
            onClick={() => setActive(os)}
            aria-pressed={active === os}
            className={`rounded border px-4 py-2 text-sm font-medium transition-colors duration-200 ${
              active === os
                ? "border-brand bg-brand text-white"
                : "border-border text-text-secondary hover:border-brand hover:text-white"
            }`}
          >
            {OS_COMMANDS[os].label.split(" (")[0]}
          </button>
        ))}
      </div>

      <pre className="mt-6 overflow-x-auto rounded-lg border border-border bg-surface-3 p-5 font-mono text-sm text-white">
        <code>{current.command}</code>
      </pre>
    </section>
  );
}