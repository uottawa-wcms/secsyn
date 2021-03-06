<?php

class SecureSyndicationPageManagerProfile extends SecureSyndicationProfile {

  public function __construct() {
    ctools_include('export');
  }

  public function package($object) {
    $package = array();
    $package['page'] = (array) $object;
    $this->packageHandlers($package);
    unset($package['page']['pid']);
    return $package;
  }

  private function packageHandlers(&$package) {
    $handlers = db_select('page_manager_handlers', 'pmh')
        ->fields('pmh')
        ->condition('task', $package['page']['task'])
        ->condition('subtask', $package['page']['name'])
        ->execute();
    $package['handlers'] = array();
    foreach ($handlers as $handler) {
      // only support panel_context for now
      if ($handler->handler == 'panel_context') {
        $packaged_handler = (array) $handler;
        $packaged_handler['conf'] = unserialize($packaged_handler['conf']);
        $this->packageHandler($packaged_handler);
        unset($packaged_handler['did']);
        $package['handlers'][$handler->weight] = $packaged_handler;
      }
    }
  }

  private function packageHandler(&$handler) {
    $display_id = $handler['conf']['did'];
    if (!empty($display_id)) {
      $display = db_select('panels_display', 'pd')
          ->fields('pd')
          ->condition('did', $display_id)
          ->execute()
          ->fetch();
      $packaged_display = (array) $display;
      $this->packageDisplay($packaged_display);
      unset($packaged_display['did']);
      $handler['_display'] = $packaged_display;
    }
    unset($handler['conf']['did']);
  }

  private function packageDisplay(&$display) {
    $display['layout_settings'] = unserialize($display['layout_settings']);
    $display['panel_settings'] = unserialize($display['panel_settings']);
    $display['cache'] = unserialize($display['cache']);
    $display['_panes'] = array();
    $panes = db_select('panels_pane', 'pp')
        ->fields('pp')
        ->condition('did', $display['did'])
        ->execute();
    foreach ($panes as $pane) {
      $packaged_pane = (array) $pane;
      $packaged_pane['access'] = unserialize($packaged_pane['access']);
      $packaged_pane['configuration'] = unserialize($packaged_pane['configuration']);
      $packaged_pane['cache'] = unserialize($packaged_pane['cache']);
      $packaged_pane['style'] = unserialize($packaged_pane['style']);
      $packaged_pane['css'] = unserialize($packaged_pane['css']);
      $packaged_pane['extras'] = unserialize($packaged_pane['extras']);
      $packaged_pane['locks'] = unserialize($packaged_pane['locks']);
      unset($packaged_pane['pid']);
      unset($packaged_pane['did']);
      $display['_panes'][] = $packaged_pane;
    }
  }

  public function update($uuid, $fields) {
    // not messy at all...
    $page = $this->lookup($uuid);
    if (!empty($page)) {
      $this->killExistingPanes($page);
    }
    db_merge('page_manager_pages')
        ->fields((array) $fields['page'])
        ->key(array(
          'name' => $fields['page']['name']
        ))
        ->execute();
    foreach ($fields['handlers'] as $handler) {
      $display = (array) $handler['_display'];
      unset($handler['_display']);
      $panes = $display['_panes'];
      unset($display['_panes']);
      drupal_write_record('panels_display', $display);
      foreach ($panes as $pane) {
        $pane = (array) $pane;
        $pane['did'] = $display['did'];
        drupal_write_record('panels_pane', $pane);
      }
      $handler['conf']['did'] = $display['did'];
      drupal_write_record('page_manager_handlers', $handler);
    }
  }

  private function findHandlers($page) {
    $page = (array) $page;
    return db_select('page_manager_handlers', 'pmh')
        ->fields('pmh')
        ->condition('task', $page['task'])
        ->condition('subtask', $page['name'])
        ->execute();
  }

  public function killExistingPanes($page) {
    $handlers = $this->findHandlers($page);
    foreach ($handlers as $handler) {
      if ($handler->handler == 'panels_context') {
        $did = $handler->conf['did'];
        db_delete('panels_pane')
            ->condition('did', $did)
            ->execute();
      }
      db_delete('panels_display')
          ->condition('did', $did)
          ->execute();
      db_delete('page_manager_handlers')
        ->condition('name', $handler->name)
        ->execute();
    }
  }

  public function lookup($uuid) {
    $page = db_select('page_manager_pages', 'pmp')
        ->fields('pmp')
        ->condition('name', $uuid)
        ->execute()
        ->fetch();
    if (empty($page)) {
      return FALSE;
    }
    return $page;
  }

  public function uuid($object) {
    if (empty($object->name)) {
      return FALSE;
    }
    return $object->name;
  }

}
