<?php

/**
 * @file
 * This is a site check that goes over the site and checks for common issues.
 */

/**
 * Global to store all the errors.
 *
 * All errors that are encountered during the site check will be added to this
 * and then made available for use for things like email reporting.
 */
global $errors;

/**
 * Output a message to the user via drush_log().
 *
 * @param string $message
 *   The message to display to the user.
 */
function output_info($message) {
  drush_log("     $message", 'ok');
}

/**
 * Output an error message via drush_log().
 *
 * This will not only output to the user but also capture all errors into a
 * variable (global $errors) which can be utilised later.
 *
 * @param string $message
 *   The error message to be shown to the user.
 */
function output_error($message) {
  drush_log("     $message", 'error');

  global $errors;
  $errors[] = $message;
}

/**
 * Get all errors that are currently available.
 *
 * @return array
 *   An array of all the errors encountered during the checks.
 */
function get_errors() {
  global $errors;
  return $errors;
}

/**
 * Output a header line in the drush line.
 *
 * @param string $message
 *   The message to output as a header.
 */
function output_header_info($message) {
  echo "\n---> $message\n";
}

/**
 * Register the drush command.
 */
function site_check_drush_command() {
  $items = array();

  $items['site-check'] = array(
    'description' => "Runs a performance audit across the whole site.",
    'options' => array(
      'email-report' => 'Email a report upon completion of the audit.',
      'email-address' => 'Address of where to send the audit report.',
      'domain' => 'Domain to include in the output. Only used if sending an email report.',
    ),
    'examples' => array(
      'drush site-check' => 'Running a site performance audit.',
      'drush site-check --email-report --email-address=test@test.com' => 'Running an audit and sending an email of the results.',
    ),
    'aliases' => array('sc'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

/**
 * Main method to execute the sub methods.
 */
function drush_site_check() {
  output_header_info("Checking environment data...");
  check_environment_options();
  output_header_info("Running module checks...");
  run_module_audit();
  output_header_info("Running checks on the caching layers...");
  run_cache_check();
  output_header_info("Checking assets and aggregation...");
  run_asset_check();
  output_header_info("Performing a security check...");
  perform_security_audit();
  output_header_info("Analysing settings.php file...");
  check_settings_file_options();

  // Fire off the email if the parameters have been set for it.
  if (drush_get_option('email-report') && drush_get_option('email-address')) {
    $email_address = drush_get_option('email-address');
    $errors = get_errors();
    $domain = drush_get_option('domain');

    // Should the errors be empty, don't bother with an email.
    if (empty($errors)) {
      return FALSE;
    }

    output_header_info("Emailing report to $email_address...");
    $headers = "From: $from\r\n" .
      "Reply-To: $from\r\n" .
      'X-Mailer: PHP/' . phpversion();
    $date = date('H:i:s - j M Y');
    $from = "Site check <site-check@$domain>";
    $subject = "[$domain] site check results";
    $body = "Hi there,\n";
    $body .= "A recent site check of $domain has found the following errors:\n\n";
    foreach ($errors as $error) {
      $body .= "- $error\n";
    }

    mail($email_address, $subject, $body, $headers);
  }
}

/**
 * Fetch the MySQL version.
 *
 * This is used because often the MySQL libraries aren't available and there
 * isn't a more reliable way of doing it.
 */
function get_mysql_version() {
  $version = db_query('SELECT VERSION() as mysql_version');
  return $version->fetchColumn();
}

/**
 * Check the environment options that Drupal uses.
 */
function check_environment_options() {
  $drupal_version = VERSION;
  $php_version = phpversion();
  $memory_limit = ini_get('memory_limit');
  $mysql_version = get_mysql_version();

  output_info("Drupal version is $drupal_version.");
  output_info("Running on PHP version $php_version.");
  output_info("PHP memory limit is $memory_limit.");
  output_info("MySQL version is $mysql_version.");

  check_temporary_file_path();
  perform_cron_check();
}

/**
 * Check to see if the temporary file path is available.
 */
function check_temporary_file_path() {
  $path = variable_get('file_temporary_path');

  if (!empty($path) && is_dir($path) && is_writable($path)) {
    output_info("The temporary file path ($path) is accessible.");
  }
  else {
    output_error("The temporary file path ($path) is not accessible.");
  }
}

/**
 * Run a check to ensure the cron has run in the last 3 hours.
 */
function perform_cron_check() {
  if (variable_get('cron_last') > strtotime('-3 hours')) {
    output_info('Cron has been run in the last 3 hours.');
  }
  else {
    output_error('Cron has not been run in the last 3 hours.');
  }
}

/**
 * Perform a module check against the installed modules.
 *
 * The module check compares the current status against the desired status.
 */
function run_module_audit() {
  $modules = array(
    array(
      'name' => 'views_ui',
      'friendly_name' => 'Views UI',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'views_php',
      'friendly_name' => 'Views PHP',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'dblog',
      'friendly_name' => 'Database logging',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'syslog',
      'friendly_name' => 'System logging',
      'desired_status' => TRUE,
    ),
    array(
      'name' => 'memcache',
      'friendly_name' => 'Memcache',
      'desired_status' => TRUE,
    ),
    array(
      'name' => 'admin',
      'friendly_name' => 'Administration tools',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'cron_debug',
      'friendly_name' => 'Cron debug',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'devel',
      'friendly_name' => 'Devel',
      'desired_status' => FALSE,
    ),
    array(
      'name' => 'imagemagick',
      'friendly_name' => 'ImageMagick',
      'desired_status' => TRUE,
    ),
    array(
      'name' => 'php',
      'friendly_name' => 'PHP filter',
      'desired_status' => FALSE,
    ),
  );

  foreach ($modules as $module) {
    $status = ($module['desired_status']) ? 'enabled' : 'disabled';

    if ($module['desired_status'] == module_exists($module['name'])) {
      output_info("{$module['friendly_name']} is $status.");
    }
    else {
      output_error("{$module['friendly_name']} should be $status.");
    }
  }

  // Check for modules that have been disabled but not uninstalled.
  $uninstalled_modules = db_select('system', 's')
    ->fields('s', array('name'))
    ->condition('status', '0', '=')
    ->condition('schema_version', array(-1, 0), 'NOT IN')
    ->execute()
    ->fetchAssoc();

  if (empty($uninstalled_modules)) {
    output_info('There are no modules pending uninstallation.');
  }
  else {
    output_error('There are modules that have not been completely uninstalled.');
  }
}

/**
 * Run a check against the caching options and caching layers.
 */
function run_cache_check() {
  $cache = array();

  if (variable_get('cache')) {
    output_info('Page caching is enabled.');
  }
  else {
    output_error('Page caching is disabled.');
  }

  if (variable_get('block_cache')) {
    output_info('Block caching is enabled.');
  }
  else {
    output_error('Block caching is disabled.');
  }

  if (variable_get('cache_lifetime') > 0) {
    output_info('Minimum cache lifetime is set.');
  }
  else {
    output_error('Minimum cache lifetime is not set.');
  }

  if (variable_get('page_cache_maximum_age') > 0) {
    output_info('External page caching is enabled.');
  }
  else {
    output_error('External page caching is disabled.');
  }
}

/**
 * Check the assets aggregation and compression.
 */
function run_asset_check() {
  $assets = array(
    array(
      'name' => 'page_compression',
      'friendly_name' => 'Page compression',
      'desired_status' => TRUE,
    ),
    array(
      'name' => 'preprocess_css',
      'friendly_name' => 'CSS aggregation',
      'desired_status' => TRUE,
    ),
    array(
      'name' => 'preprocess_js',
      'friendly_name' => 'JS aggregation',
      'desired_status' => TRUE,
    ),
  );

  foreach ($assets as $asset) {
    $status = ($asset['desired_status']) ? 'enabled' : 'disabled';

    if ($asset['desired_status'] == variable_get($asset['name'])) {
      output_info("{$asset['friendly_name']} is $status.");
    }
    else {
      output_error("{$asset['friendly_name']} should be $status.");
    }
  }

  // Check if Zen is rebuilding the theme registry on every request.
  foreach (list_themes() as $name => $theme) {
    if (isset($theme->status) && $theme->status) {
      if (theme_get_setting('zen_rebuild_registry', $name)) {
        output_error("Zen is rebuilding the theme registry for '{$theme->name}' theme on every request.");
      }
    }
  }
}

/**
 * Run a security audit for potential security issues.
 */
function perform_security_audit() {
  // Ensure the root user is disabled.
  $root_user_check = db_select('users', 'u')
    ->fields('u', array('uid', 'status'))
    ->condition('uid', 1, '=')
    ->execute()
    ->fetchAssoc();

  if ($root_user_check['status'] == 0) {
    output_info('User ID 1 is disabled.');
  }
  else {
    output_error('User ID 1 should be blocked from access.');
  }

  // Make sure error reporting is not visible to end users.
  if (variable_get('error_level') == 0) {
    output_info('Error reporting is not being shown to end users.');
  }
  else {
    output_error('Error reporting is being shown to end users.');
  }
}

/**
 * Ensure the settings files contain some required lines.
 */
function check_settings_file_options() {
  $settings_path = conf_path() . '/settings.php';
  $settings = file_get_contents($settings_path);

  // Ensure update.php is not accessible when a user is not authenticated.
  global $update_free_access;
  if ($update_free_access == FALSE) {
    output_info('Update.php requires authenticated access to be run.');
  }
  else {
    output_error('Update.php is accessible to anonymous users.');
  }

  // Fast 404's prevent unnessecary bootstrap calls from being executed on a
  // missing imagge or asset.
  if (preg_match('/(# drupal_fast_404)/', $settings)) {
    output_error('Fast 404\'s is disabled.');
  }
  else {
    output_info('Fast 404\'s are enabled.');
  }
}
