import type { Command } from "../types.js";
import { balance } from "./balance.js";
import { link } from "./link.js";
import { premium } from "./premium.js";
import { status } from "./status.js";

/** All Phase-1 slash commands, keyed by their command name. */
export const commands: Command[] = [link, balance, status, premium];

export const commandMap = new Map<string, Command>(
  commands.map((command) => [command.data.name, command]),
);