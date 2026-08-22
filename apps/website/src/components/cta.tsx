import Link from "next/link";

export function Cta() {
  return (
    <section className="border-t border-border" aria-labelledby="cta-heading">
      <div className="mx-auto flex max-w-6xl flex-col items-center gap-6 px-6 py-24 text-center">
        <h2
          id="cta-heading"
          className="text-3xl font-extrabold tracking-tight text-white md:text-4xl"
        >
          Ready to start <span className="text-brand">playing</span>?
        </h2>
        <p className="max-w-xl text-lg text-text-secondary">
          Download Pk_Launcher for free and jump into your favourite Minecraft
          version in minutes.
        </p>
        <div className="mt-2 flex flex-wrap items-center justify-center gap-4">
          <Link
            href="/download"
            className="rounded bg-brand px-8 py-3 text-sm font-bold text-white transition-colors duration-200 hover:bg-brand-red-hover shadow-glow-sm"
          >
            Download now
          </Link>
          <Link
            href="/login"
            className="rounded border border-border px-8 py-3 text-sm font-semibold text-white transition-colors duration-200 hover:border-brand hover:text-brand"
          >
            Sign in
          </Link>
        </div>
      </div>
    </section>
  );
}