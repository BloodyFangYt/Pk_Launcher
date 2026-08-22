import { EmbedBuilder, SlashCommandBuilder } from "discord.js";
import { ApiError } from "../backend/client.js";
import type { Command } from "../types.js";

/** /link — link a PK Launcher account to Discord with a one-time code. */
export const link: Command = {
  data: new SlashCommandBuilder()
    .setName("link")
    .setDescription(
      "Link your PK Launcher account to Discord using a one-time code.",
    )
    .addStringOption((option) =>
      option
        .setName("code")
        .setDescription("One-time linking code shown in the PK Launcher app")
        .setRequired(true),
    ),
  async execute(interaction, api) {
    const code = interaction.options.getString("code", true).trim();
    await interaction.deferReply({ ephemeral: true });

    try {
      if (code.length === 0) {
        await interaction.editReply({ content: "❌ The code cannot be empty." });
        return;
      }
      const account = await api.link({ discordId: interaction.user.id, code });

      const embed = new EmbedBuilder()
        .setTitle("✅ Account linked")
        .setDescription(
          account.username
            ? `Linked **${account.username}** to <@${interaction.user.id}>!`
            : `Linked your PK Launcher account to <@${interaction.user.id}>!`,
        )
        .setColor("Green");
      await interaction.editReply({ embeds: [embed] });
    } catch (error) {
      const message =
        error instanceof ApiError
          ? error.message
          : "Linking failed. Please try again later.";
      await interaction.editReply({ content: `❌ ${message}` });
    }
  },
};