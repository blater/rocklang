const { Plugin } = require("obsidian");

module.exports = class AsmAsTextPlugin extends Plugin {
  async onload() {
    this.registerExtensions(["asm"], "markdown");
  }
};
