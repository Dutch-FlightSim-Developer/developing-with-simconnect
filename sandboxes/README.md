# Sandboxes

This directory is a placeholder for local-only experimental projects.

The `CMakeLists.txt` is included in the top-level build so that sandbox programs
are built alongside everything else. Add your experiments by creating subdirectories
here and registering them with `add_subdirectory()` in `CMakeLists.txt`.

## What is and isn't tracked

- `CMakeLists.txt`, `.gitignore`, and this `README.md` are committed to the repository.
- Everything else — subdirectories, source files, binaries — is gitignored and stays local.
- Changes to `CMakeLists.txt` itself are also suppressed via `git update-index --skip-worktree`,
  so adding `add_subdirectory` lines for your sandboxes will never show up as staged changes.

## After cloning or pulling on a new machine

Run this once to suppress local changes to `CMakeLists.txt`:

```bash
git update-index --skip-worktree sandboxes/CMakeLists.txt
```

Without this, any edits you make to `CMakeLists.txt` (e.g. adding your sandbox targets)
will appear as uncommitted changes in `git status`.
