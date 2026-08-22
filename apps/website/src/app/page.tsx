import { Cta } from "@/components/cta";
import { Features } from "@/components/features";
import { Footer } from "@/components/footer";
import { Navbar } from "@/components/navbar";
import { Screenshots } from "@/components/screenshots";
import Link from "next/link";

export default function HomePage() {
  return (
    <>
      <Navbar />
      <main>
        <section className="mx-auto max-w-6xl px-6 pt-24 pb-20 text-center">
          <p className="text-sm font-medium tracking-widest text-text-accent uppercase">
            Minecraft launcher
          </p>
          <h1 className="mx-auto mt-4 max-w-3xl text-4xl font-black leading-tight tracking-tight text-white md:text-6xl">
            The fast, modern way to{" "}
            <span className="text-brand">play Minecraft</span>
          </h1>
          <p className="mx-auto mt-6 max-w-2xl text-lg text-text-secondary">
            Download, manage versions, install mods and keep your account in sync —
            all from one sleek, secure launcher.
          </p>
          <div className="mt-10 flex flex-wrap items-center justify-center gap-4">
            <Link
              href="/download"
              className="rounded bg-brand px-8 py-3 text-sm font-bold text-white transition-colors duration-200 hover:bg-brand-red-hover shadow-glow-sm"
            >
              Download for free
            </Link>
            <Link
              href="/store"
              className="rounded border border-border px-8 py-3 text-sm font-semibold text-white transition-colors duration-200 hover:border-brand hover:text-brand"
            >
              Browse the store
            </Link>
          </div>
        </section>

        <Features />
        <Screenshots />
        <Cta />
      </main>
      <Footer />
    </>
  );
}