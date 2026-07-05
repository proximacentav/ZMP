# Git Mirror Setup

## Установка

1. Скопируйте `push-mirror.sh` в ваш PATH (например, `/usr/local/bin/`):
```bash
sudo cp push-mirror.sh /usr/local/bin/
sudo chmod +x /usr/local/bin/push-mirror.sh
```

2. Добавьте alias в `~/.gitconfig` (или `~/.config/git/config`):
```bash
git config --global alias.push-all '!push-mirror.sh'
```

## Использование

### Способ 1: Git alias (рекомендуется)
```bash
git push-all [ветка]
# По умолчанию используется ветка main
```

### Способ 2: Прямой вызов скрипта
```bash
./push-mirror.sh [ветка]
```

### Способ 3: Git hook (автоматически)
После настройки alias, при использовании `git push-all` скрипт автоматически отправит изменения на GitHub и GitVerse.

## Конфигурация

Скрипт использует два remote:
- `origin` - GitHub (https://github.com/proximacentav/ZMP.git)
- `gitverse` - GitVerse (https://gitverse.ru/proximacentav/ZMP.git)

## Примеры

```bash
# Отправить main ветку
git push-all

# Отправить другую ветку
git push-all feature-branch
```
