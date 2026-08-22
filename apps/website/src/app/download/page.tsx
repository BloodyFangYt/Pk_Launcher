import { Cta } from "@/components/cta";
import { DownloadCommands } from "@/components/download-commands";
import { Footer } from "@/components/footer";
import { Navbar } from "@/components/navbar";
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Download",
  description:
    "Download the Pk_Launcher for Windows, macOS and Linux — free and open source.",
};

const VERSIONS = [
  {
    os: "Windows",
    arch: "x64",
    size: "~45 MB",
    notes: "Windows 10/11, 64-bit",
  },
  {
    os: "macOS",
    arch: "Apple Silicon",
    size: "~48 MB",
    notes: "macOS 13+",
  },
  {
    os: "Linux",
    arch: "x64 (AppImage)",
    size: "~44 MB",
    notes: "GNOME/KDE recommended",
  },
] as const;

export default function DownloadPage() {
  return (
    <>
      <Navbar />
      <main className="mx-auto max-w-6xl px-6 py-20">
        <div className="mx-auto max-w-2xl text-center">
          <p className="text-sm font-medium tracking-widest text-text-accent uppercase">
            Get started
          </p>
          <h1 className="mt-4 text-4xl font-extrabold tracking-tight text-white md:text-5xl">
            Download <span className="text-brand">Pk_Launcher</span>
          </h1>
          <p className="mt-4 text-lg text-text-secondary">
            Free for Windows, macOS and Linux. Jump straight into your favourite
            Minecraft version.
          </p>
        </div>

        <div className="mt-16 grid gap-6 md:grid-cols-3">
          {VERSIONS.map((version) => (
            <article
              key={`${version.os}-${version.arch}`}
              className="flex flex-col rounded-xl border border-border bg-surface-2 p-6 transition-all duration-200 hover:border-brand hover:shadow-glow-sm"
            >
              <h2 className="text-xl font-bold text-white">{version.os}</h2>
              <p className="mt-1 text-sm font-medium text-text-accent">
                {version.arch}
              </p>
              <p className="mt-3 text-sm text-text-muted">{version.notes}</p>
              <p className="mt-1 text-sm text-text-muted">
                ~{version.size} download
              </p>
              <span className="mt-6 inline-flex items-center justify-center rounded bg-brand px-4 py-2.5 text-sm font-semibold text-white transition-colors duration-200 hover:bg-brand-red-hover">
                Download for {version.os} →
              </span>
            </article>
          ))}
        </div>

        <DownloadCommands />
      </main>
      <Cta />
      <Footer />
    </>
  );
}