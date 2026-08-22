import { Footer } from "@/components/footer";
import { Navbar } from "@/components/navbar";
import type { Metadata } from "next";

export const metadata: Metadata = {
  title: "Login",
  description: "Sign in to your Pk_Launcher account.",
};

export default function LoginPage() {
  return (
    <>
      <Navbar />
      <main className="mx-auto flex max-w-md flex-col px-6 py-20">
        <div className="text-center">
          <p className="text-sm font-medium tracking-widest text-text-accent uppercase">
            Welcome back
          </p>
          <h1 className="mt-4 text-3xl font-extrabold tracking-tight text-white md:text-4xl">
            Sign in to <span className="text-brand">Pk_Launcher</span>
          </h1>
          <p className="mt-3 text-text-secondary">
            Your launcher, store and Discord account all in one place.
          </p>
        </div>

        <form className="mt-10 space-y-5">
          <label className="block">
            <span className="text-sm font-medium text-text-secondary">Email</span>
            <input
              type="email"
              name="email"
              required
              autoComplete="email"
              className="mt-2 w-full rounded-lg border border-border bg-surface-3 px-4 py-3 text-white placeholder:text-text-muted focus:border-brand focus:ring-2 focus:ring-brand/20 focus:outline-none"
              placeholder="you@example.com"
            />
          </label>

          <label className="block">
            <span className="text-sm font-medium text-text-secondary">
              Password
            </span>
            <input
              type="password"
              name="password"
              required
              autoComplete="current-password"
              className="mt-2 w-full rounded-lg border border-border bg-surface-3 px-4 py-3 text-white placeholder:text-text-muted focus:border-brand focus:ring-2 focus:ring-brand/20 focus:outline-none"
              placeholder="••••••••"
            />
          </label>

          <button
            type="submit"
            className="w-full rounded-lg bg-brand px-4 py-3 text-sm font-bold text-white transition-colors duration-200 hover:bg-brand-red-hover"
          >
            Sign in
          </button>
        </form>

        <p className="mt-8 text-center text-sm text-text-muted">
          New to Pk_Launcher?{" "}
          <a
            href="/login"
            className="font-medium text-brand transition-colors duration-200 hover:text-brand-red-light"
          >
            Create an account
          </a>
        </p>
      </main>
      <Footer />
    </>
  );
}