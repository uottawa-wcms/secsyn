<?php


class SecureSyndicationFileProfile extends SecureSyndicationProfile {

  public function package($object) {
    $object['file_contents'] = gzcompress(file_get_contents(
        $this->getAbsolutePath($object)
    ));
  }

  public function update($uuid, $fields) {
    $object = $this->buildFileObject($uuid);
    $path = $this->getAbsolutePath($object);
    file_put_contents($path, gzdeflate($fields['file_contents']));

  }

  public function lookup($uuid) {
    $object = $this->buildFileObject($uuid);
    if (file_exists($this->getAbsolutePath($object))) {
      return $object;
    }
    return NULL;
  }

  public function uuid($object) {
    if ($object['schema'] == 'public' ||
        $object['schema'] == 'private') {
      return $object['schema'] . '://' . $object['path'];
    } else {
      return 'http://SERVER' . $object['path'];
    }
  }

  public function hash($object) {
    return hash_file('sha256', $this->getAbsolutePath($object));
  }

  private function buildFileObject($uuid) {
    $pieces = parse_url($uuid);
    return array(
      'path' => $pieces['path'],
      'schema' => file_uri_scheme($uuid),
    );
  }

  private function getAbsolutePath($object) {
    $file = $this->uuid($object);
    $wrapper = file_stream_wrapper_get_instance_by_uri($file);
    return $wrapper->realpath();
  }
}

?>
