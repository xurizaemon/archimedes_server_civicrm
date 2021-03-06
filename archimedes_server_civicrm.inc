<?php

/**
 * Plugin to parse document item into a CiviCRM Extension node.
 */
function archimedes_server_civicrm_plugin_civicrm_extension(DomElement $item, $field_name, $ns) {

  watchdog('archimedes_server_civicrm', 'archimedes_server_civicrm_plugin_civicrm_extension: item:<pre>@item</pre>, field_name:<pre>@field_name</pre>, ns:<pre>@ns</pre>,', array('@item' => print_r($item, 1), '@field_name' => print_r($field_name, 1), '@ns' => print_r($ns, 1)));

    global $user;

    $node = new StdClass();
    $node->uid = $user->uid;
    $node->status = NODE_PUBLISHED;
    $node->type = 'project';
    $node->language = LANGUAGE_NONE;

    $fields = array(
      'body',
      'field_name',
      'field_dru_proj',
    );

    foreach ($fields as $field) {
        if ($value = trim($item->getAttributeNodeNS($ns, $field)->nodeValue)) {
            $node->{$field}[LANGUAGE_NONE][0]['value'] = $value;
        }
    }

    if ($version = trim($item->getAttributeNodeNS($ns, 'field_mod_version')->nodeValue)) {
        //watchdog('archimedes_server', '@name @version', array('@name' => $node->field_name[LANGUAGE_NONE][0]['value'], '@version' => $version), WATCHDOG_DEBUG);
        if (strpos($version, '.') === FALSE) {
            // Drush may set a version like 'master' or 'UNKNOWN' here, but
            // we're going to need some Drupal version for storage. That
            // field is currently an integer, so we use 0 to prevent the
            // storage for this site bombing.
            //
            // https://github.com/xurizaemon/archimedes_server
            $drupal_version = 0;
            $module_version = $major_version = $version;
            $minor_version = '';
        }
        elseif (strpos($version, '-') === FALSE) {
            $drupal_version = $module_version = $version;
            list($major_version, $version_patch) = explode('.', $drupal_version);
        }
        else {
            list($drupal_version, $module_version) = explode('-', $version, 2);
            @list($major_version, $version_patch) = explode('.', $module_version, 2);
        }
        list($drupal_compat,) = explode('.', $drupal_version);

        $node->field_drupal_compat[LANGUAGE_NONE][0]['value'] = $drupal_compat;

        if (is_numeric($major_version)) {
            $node->field_major_version[LANGUAGE_NONE][0]['value'] = $major_version;
        }
        else {
            $version_patch = $major_version;
        }
        $node->field_version_patch[LANGUAGE_NONE][0]['value'] = $version_patch;
    }

    $node->body[LANGUAGE_NONE][0]['format'] = 'full_html';

    $node->title = trim(t('@name @version', array(
      '@name' => trim($item->getAttributeNodeNS($ns, 'title')->nodeValue),
      '@version' => isset($version) ? $version : '',
    )));

    // Look for an existing project.
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
      ->propertyCondition('title', $node->title)
      ->propertyCondition('type', $node->type)
      ->fieldCondition('field_name', 'value', $node->field_name[LANGUAGE_NONE][0]['value']);
    if (isset($node->field_major_version, $node->field_version_patch, $node->field_drupal_compat)) {
        $query->fieldCondition('field_drupal_compat', 'value', $node->field_drupal_compat[LANGUAGE_NONE][0]['value']);
        $query->fieldCondition('field_major_version', 'value', $node->field_major_version[LANGUAGE_NONE][0]['value']);
        $query->fieldCondition('field_version_patch', 'value', $node->field_version_patch[LANGUAGE_NONE][0]['value']);
    }
    $ids = $query->execute();

    // If a node already exists. Use that.
    if (isset($ids['node']) && count($ids['node'])) {
        return node_load(key($ids['node']));
    }

    // Otherwise, save the node.
    node_save($node);

    return $node;
}
