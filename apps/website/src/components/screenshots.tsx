const SCREENSHOTS = [
  {
    label: "Launch screen",
    title: "One-click launch",
    description: "Pick a version, allocate your RAM and hit play.",
  },
  {
    label: "Version list",
    title: "Every version, managed.",
    description: "Snapshots, releases and modded instances in one clear list.",
  },
  {
    label: "Mod loader",
    title: "Mods made simple.",
    description: "Install Forge, Fabric or Quilt without touching a file manager.",
  },
] as const;

export function Screenshots() {
  return (
    <section
      className="border-y border-border bg-surface-1"
      aria-labelledby="screenshots-heading"
    >
      <div className="mx-auto max-w-6xl px-6 py-24">
        <div className="mx-auto max-w-2xl text-center">
          <h2
            id="screenshots-heading"
            className="text-3xl font-extrabold tracking-tight text-white md:text-4xl"
          >
            See it in action
          </h2>
          <p className="mt-4 text-lg text-text-secondary">
            A clean, fast interface designed around the way you play.
          </p>
        </div>

        <div className="mt-16 grid gap-6 lg:grid-cols-3">
          {SCREENSHOTS.map((shot) => (
            <div
              key={shot.title}
              className="rounded-xl border border-border bg-surface-0 p-6"
            >
              <div className="flex aspect-video items-center justify-center rounded-lg border border-dashed border-border bg-surface-3 text-text-muted">
                {shot.label}
              </div>
              <h3 className="mt-5 text-lg font-semibold text-white">{shot.title}</h3>
              <p className="mt-1 text-sm text-text-secondary">{shot.description}</p>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}