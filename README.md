# Drupal site check

This is a drush command that is able to check for common site issues and
assist in running a performance savvy Drupal website.

## Installation

```bash
cd ~/.drush
git clone https://github.com/jacobbednarz/site-check.git
drush cache-clear drush
```

## Usage

To invoke the site check command, you can run `drush site-check` either in your
site directory or using a predefined drush alias.

The site check also enables you pass in an email address to have a report sent
upon completion. Example, to send an email to `example@domain.com` you would
do the send the following command:

```bash
drush site-check --email-report --email-address=example@domain.com
```

## What is included?

Out of the box, this ships with:

- Environmental data information such as PHP verison and MySQL version.
- Ensuring cron has been run recently (within a few hours).
- Checks for known performance hindering modules such as [devel](drupal.org/project/devel).
- Information on your cache settings and aggregation options.
- Security checks for common exploits and potential security concerns.
- Analysis of the settings.php files to ensure safer defaults are enabled.

However, any and all of these can be changed and altered to better suit your
needs or site requirements.

## Contributing

See [CONTRIBUTING](contributing.md)
