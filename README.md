![Screenshot](/img/screenshot.png)

- Live Wasm [Demo](https://memononen.github.io/condui/imgui.html)

# Condition UI example in Dear ImGui

This is an example of boolean expression editor.

Each condition is at it's own row. The operator is applied between the current and previous condition. No operator precedence, operations are combined in order.

Each line can be indented to create more complicated expressions.

Diff between the idents defines how many braces open or close between the condition lines. The more you indent, the more the line will be associated with the previous line.

## Motivation

Boolean expression UIs are needed in many applications, like search queries, or conditions in game logic (transitions, enter conditions, etc). Common approaches are to make such UI is to use a treeview using prefix or python style all/any containers, or some custom horizontal editor.

They both require complicated logic, and refactoring an expression becomes really hard, depending on the UI framework, you may be required to redo the whole thing, if you want to encapsulate a branch.

Complex conditions are hard to express even in code, which is usually the most versatile format. There is certain cap how much indentation a person can handle when trying to decipher such condition.

Another thing to consider is that a lot of game engine editors are based on property grids, where each property is shown at separate line. Even if that was not an issue, the space for editing the conditions is usually very narrow, so a solution that expands vertically is going to work there better.

This solutions aims to be quick to use, legible, vertical, and user proof. For example solutions that would deal with parentheses directly, could create expressions that are not valid. This method creates always valid expressions, even if it might not be correct (e.g. during array reorder).

---

MIT license

Code based on [WebGui](https://github.com/jnmaloney/WebGui) sample project.
