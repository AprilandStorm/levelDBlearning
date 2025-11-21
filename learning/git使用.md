# 场景 1：完全切换到新仓库（以后只往新仓库推拉）
- 步骤 1：获取新仓库的地址（SSH/HTTPS 二选一）\
打开你的 新 GitHub 仓库（比如 new-leveldb-repo）；\
点击右上角 Code → 复制地址：\
推荐 SSH 地址（免密码，需新仓库所属账号绑定你的 SSH 密钥）：git@github.com:AprilandStorm/new-leveldb-repo.git；\
备选 HTTPS 地址（需输账号 + 令牌）：https://github.com/AprilandStorm/new-leveldb-repo.git
- 步骤 2：修改本地的远程仓库关联（替换默认的 origin）
先删除旧的远程关联（指向 levelDBlearning），再关联新仓库：
```
//1. 查看当前的远程关联（确认有旧的 origin 指向旧仓库）
git remote -v
//2. 删除旧的远程关联（删除后，origin 别名被释放）
git remote rm origin
//3. 关联新仓库到 origin 别名（默认别名，后续推拉直接用 git pull/push）
git remote add origin git@github.com:AprilandStorm/new-leveldb-repo.git  //替换为你的新仓库 SSH 地址
//4. 验证关联是否正确（输出新仓库地址即为成功）
git remote -v
```
正确输出示例：
```
origin  git@github.com:AprilandStorm/new-leveldb-repo.git (fetch)
origin  git@github.com:AprilandStorm/new-leveldb-repo.git (push)
```
- 步骤 3：推拉新仓库的代码（和之前操作完全一致）\
拉取新仓库的内容（如果新仓库有 README 等文件，避免冲突）：
```
git pull origin main --allow-unrelated-histories  # 首次拉取需加 --allow-unrelated-histories（无共同历史）
```
推送本地代码到新仓库：
```
git push -u origin main  # 绑定本地 main 和新仓库的 main，后续直接用 git push
```

# 场景 2：同时管理多个仓库（旧仓库 + 新仓库，分别推拉）
适合需求：需要保留旧仓库的关联（偶尔推拉），同时新增新仓库的关联（主要提交）。此时不用删除旧关联，而是给新仓库起一个「新的远程别名」（比如 new-origin），通过别名区分推拉目标。
- 步骤 1：给新仓库添加「新的远程别名」
不用删除旧的 origin（指向旧仓库），直接新增一个别名（比如 new-origin）关联新仓库：
```
1. 新增远程别名 new-origin，指向新仓库（别名可自定义，比如 new-repo、my-new-db 等）
git remote add new-origin git@github.com:AprilandStorm/new-leveldb-repo.git  # 替换为新仓库地址
2. 查看所有远程关联（此时有两个别名：origin 指向旧仓库，new-origin 指向新仓库）
git remote -v
```
正确输出示例：
```
origin      git@github.com:AprilandStorm/levelDBlearning.git (fetch)
origin      git@github.com:AprilandStorm/levelDBlearning.git (push)
new-origin  git@github.com:AprilandStorm/new-leveldb-repo.git (fetch)
new-origin  git@github.com:AprilandStorm/new-leveldb-repo.git (push)
```
- 步骤 2：通过「别名」指定推拉目标
拉取新仓库的内容：
```
git pull new-origin main --allow-unrelated-histories
```
推送代码到新仓库：
```
git push -u new-origin main  # 绑定本地 main 和 new-origin 的 main，后续推送新仓库直接用 git push new-origin
```
若需推拉旧仓库，仍用原来的别名：
```
git pull origin main  # 拉取旧仓库
git push origin main  # 推送旧仓库
```
# 关键说明（避免踩坑）
- 远程别名的作用：别名是 Git 给远程仓库起的 “昵称”，默认 origin 是「主仓库」的约定俗成别名，可自定义（如 new-origin、backup-repo），方便区分多个远程仓库。
- SSH 密钥的兼容性：只要新仓库属于 你当前的 GitHub 账号（AprilandStorm），且该账号已绑定你的虚拟机 SSH 密钥，就无需重新生成密钥，直接用 SSH 地址推拉即可；若新仓库属于其他 GitHub 账号，需给该账号也添加你的 SSH 密钥（或改用 HTTPS 地址）。
- 首次拉取新仓库的必加参数：本地仓库和新仓库是 “独立创建的，无共同历史”，首次 git pull 必须加 --allow-unrelated-histories，否则 Git 会拒绝合并。
- 查看 / 删除 / 修改远程关联的常用命令：

|命令|作用|
|----|----|
|git remote -v|查看所有远程仓库关联（别名 + 地址）|
|git remote add 别名 仓库地址|新增远程关联|
|git remote rm 别名|删除指定远程关联|
|git remote set-url 别名 新地址|直接修改已有别名的仓库地址（无需删除再添加）|

# 参数

|参数|全称|核心作用|使用场景|示例|
|----|----|----|----|----|
|-u|--set-upstream|绑定「本地分支」和「远程分支」，后续可简化命令|首次推送分支到远程时使用|git push -u origin main：
1. 推送本地 main 到远程 origin 的 main；
2. 绑定后，下次直接用 git push（无需再写 origin main）。|
|-v|--verbose|显示「详细输出」（默认是简洁输出），用于查看更多细节|查看远程仓库关联、调试命令时使用	|git remote -v：
1. 不加 -v 只显示远程别名（如 origin）；
2. 加 -v 显示别名对应的完整仓库地址（fetch 拉取地址、push 推送地址）。|

- 推送 / 拉取相关参数

参数（短 / 长）	全称	核心作用	使用场景	示例
-f	--force	强制推送 / 拉取（覆盖远程或本地内容）	远程内容无用，需覆盖时（谨慎使用）	git push -u origin main --force：
强制用本地 main 覆盖远程 main（删除远程本地没有的内容）。
--allow-unrelated-histories	无（长参数）	允许合并「无共同提交历史」的两个分支（本地和远程独立创建时）	首次拉取远程，提示 “无相关历史” 时	git pull origin main --allow-unrelated-histories：
解决你之前 “本地和远程仓库独立创建” 的合并问题。

- 查看信息相关参数

|参数（短 / 长）|全称|核心作用|使用场景|示例|
|----|----|----|----|----|
|-a|--all|显示所有相关内容（如所有分支、所有远程）|查看所有分支、所有远程关联时|git branch -a：查看本地 + 远程的所有分支；
git remote -v -a（等价 git remote -v）。|
|-l|--list|列出指定内容（如分支、标签）|列出分支、标签时（默认可省略）|git branch -l：列出本地所有分支（和 git branch 效果一致）。|

- 提交 / 合并相关参数

|参数（短 / 长）|全称|核心作用|使用场景|示例|
|----|----|----|----|----|
|-m|--message|直接指定提交信息（无需弹出编辑器）|提交代码时快速写说明|git commit -m "add test.cc and .gitignore"：
提交暂存区文件，附带提交信息（描述本次修改）。|
|--abort|无（长参数）|终止当前的合并 / 拉取操作，恢复到操作前的状态|合并冲突无法解决，想放弃时|git merge --abort：
放弃之前未完成的合并，清除冲突状态。|

- 远程仓库相关参数

|参数（短 / 长）|全称|核心作用|使用场景|示例|
|----|----|----|----|----|
|-add|无（子命令参数）|新增远程仓库关联（给远程仓库起别名）|首次关联远程仓库时|git remote add origin 仓库地址：
新增 origin 别名，绑定目标仓库。|
|-rm|--remove|删除指定的远程仓库关联|更换远程仓库，删除旧别名时|git remote rm origin：
删除之前绑定的 origin 别名。|
|-set-url|无（长参数）|修改已有远程别名的仓库地址（无需删除再添加）|远程仓库地址变更时|git remote set-url origin 新仓库地址：
直接修改 origin 对应的仓库地址（比如切换到新仓库）。|
