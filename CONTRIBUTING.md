## Contribution Guidelines
As the repository is currently maintained by one person (hello, that's me!) there aren't any strict guidelines for contributing. I'd be really happy if you fix any bugs you can spot. Write with K&R style, and try to fit the general style of the code around it.

## Contributing Guide
Assuming you're on GitHub, you must first fork the repository to your own account by pressing the fork button. 
![Photo of the fork button](https://i.imgur.com/OWNTQDM.png)
After that, go to the directory where you want to do your edits and run:
```bash
git clone https://github.com/YOUR-GITHUB-USERNAME/cpersist
```
This will download the code into your device. Then run:
```bash
git remote add upstream https://github.com/Mher-DeLight/cpersist
git switch -c thing-youre-adding
```
This will create a new branch. You should name the branch based on the feature you're adding such that it is easy to tell what it's supposed to do. Making a new branch will make it that your changes don't directly affect the main branch if you mess something up.

After that, you should make your changes. During those changes, you should periodically commit your changes via:
```bash
git add -A
git commit -m "commit message here"
```
When you finish adding and testing your features, commit once again with the commands above, then run:
```bash
git push origin thing-youre-adding
```
After that, go to GitHub again and open your fork's page. You will see a button labeled "Compare & pull request". Press it. Then describe what you changed, and if you fixed an issue, give the issue ID. Then I will personally review your pull request. I might ask for some changes, you should hopefully follow. Then you should push again and the PR will automatically update. Then eventually I will accept your PR and merge it into the repository.