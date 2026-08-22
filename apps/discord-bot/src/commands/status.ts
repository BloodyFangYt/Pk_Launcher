import { EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { ApiError } from "../backend/client.js";
import type { Command } from "../types.js";

/** /status — account link + role/plan status. */
export const status: Command = {
  data: new SlashCommandBuilder()
    .setName("status")
    .setDescription("View your PK Launcher account status."),
  async execute(interaction, api) {
    if (!interaction.inCachedGuild()) {
      await interaction.reply({
        content: "❌ Please run this command in a server.",
        ephemeral: true,
      });
      return;
    }
    await interaction.deferReply({ ephemeral: true });

    try {
      const account = await api.me(interaction.user.id);
      const embed = new EmbedBuilder()
        .setTitle("📊 Account status")
        .addFields([
          { name: "Linked", value: account.linked ? "✅ Yes" : "❌ No", inline: true },
          { name: "Role", value: account.role ?? "—", inline: true },
          { name: "Linked since", value: account.linkedAt ?? "—", inline: true },
        ])
        .setColor(account.linked ? "#00b4d8" : "#6b7280");

      if (!account.linked) {
        embed.setDescription("Use `/link` with a code from the launcher.");
      }
      await interaction.editReply({ embeds: [embed] });
    } catch (error) {
      const message =
        error instanceof ApiError
          ? error.message
          : "Could not fetch your status. Please try again later.";
      await interaction.editReply({ content: `❌ ${message}` });
    }
  },
};