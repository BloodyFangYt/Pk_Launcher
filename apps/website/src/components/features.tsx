const FEATURES = [
  {
    title: "Fast Downloads",
    description:
      "Parallel asset and library downloads with resume support get you into the game faster than ever.",
  },
  {
    title: "Version Management",
    description:
      "Install and switch between Minecraft versions, snapshots and mod-loaders in a single click.",
  },
  {
    title: "Mod Loader Support",
    description:
      "Native support for Forge, Fabric and Quilt with automatic dependency resolution.",
  },
  {
    title: "Smart Java Detection",
    description:
      "Auto-detects the correct Java runtime for each version and downloads it if missing.",
  },
  {
    title: "Account Sync",
    description:
      "One account across the launcher, website and Discord bot with secure session handling.",
  },
  {
    title: "Theme Store",
    description:
      "Customise your launcher with skins and themes from the integrated store.",
  },
] as const;

export function Features() {
  return (
    <section className="mx-auto max-w-6xl px-6 py-24" aria-labelledby="features-heading">
      <div className="mx-auto max-w-2xl text-center">
        <h2
          id="features-heading"
          className="text-3xl font-extrabold tracking-tight text-white md:text-4xl"
        >
          Everything you need to <span className="text-brand">play</span>
        </h2>
        <p className="mt-4 text-lg text-text-secondary">
          Pk_Launcher brings together downloads, mods, accounts and theming in one
          fast, secure launcher.
        </p>
      </div>

      <div className="mt-16 grid gap-6 sm:grid-cols-2 lg:grid-cols-3">
        {FEATURES.map((feature) => (
          <article
            key={feature.title}
            className="rounded-xl border border-border bg-surface-2 p-6 transition-all duration-200 hover:border-brand hover:shadow-glow-sm"
          >
            <h3 className="text-lg font-semibold text-white">{feature.title}</h3>
            <p className="mt-2 text-sm leading-relaxed text-text-secondary">
              {feature.description}
            </p>
          </article>
        ))}
      </div>
    </section>
  );
}