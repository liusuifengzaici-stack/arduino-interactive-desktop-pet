# Publication Record

Published on **2026-11-01**: [arduino-interactive-desktop-pet](https://github.com/liusuifengzaici-stack/arduino-interactive-desktop-pet), with **public** visibility authorized by the project owner. The initial commit is `926857a` — `Initial commit: DesktopPet V14.1 Arduino project`.

Only the reviewed project directory was uploaded. The original private sketch, backup and machine-specific verification output remain outside the repository. No license has been chosen or added. Public visibility does not give visitors direct write access to this repository.

## Reference: publication procedure

The repository above has already been created and the initial commit pushed. The following procedure is retained as a reference for preparing a separate repository; do not recreate or overwrite the published one.

1. Sign into the intended GitHub account and check whether `arduino-interactive-desktop-pet` already exists. If it does, stop and inspect it; do not overwrite it or force-push.
2. Create an empty repository named `arduino-interactive-desktop-pet`. Public visibility is authorized by the owner. Do not initialize it with a README, `.gitignore` or license, because the prepared local repository already has its initial commit.
3. In a terminal opened inside this prepared repository, substitute the intended GitHub username in the commands below:

```sh
git remote add origin https://github.com/YOUR-GITHUB-USERNAME/arduino-interactive-desktop-pet.git
git push -u origin main
```

Authenticate through the normal GitHub/Git credential flow; never put a token into a source file or remote URL. If `origin` already exists, inspect it with `git remote -v` before making changes.

4. On GitHub, inspect the README, all four diagrams, the sketch and testing record. Check that no private backup or local logs appear in the repository or commit metadata.
5. Add authentic media and choose a license only after the owner decides. Keep the music implementation's attribution and limitations visible.

## Local-only material

The original `11.ino`, its backup and raw build logs deliberately remain outside the prepared repository. Do not upload the entire parent folder. The `.gitignore` adds protection against common local/generated files, but a staged-file review is still necessary.
